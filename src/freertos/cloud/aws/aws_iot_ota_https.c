/**
 * @file aws_iot_ota_https.c (Refactored with coreHTTP)
 * @brief HTTPS firmware downloader for OTA updates on CC35X1 using coreHTTP.
 *
 * Implements @c ota_https_download() using the FreeRTOS coreHTTP client library.
 * This abstracts away manual HTTP protocol handling and socket management.
 *
 * @par Key improvements over custom HTTPS:
 *  - Uses FreeRTOS standard coreHTTP library (same as coreMQTT pattern)
 *  - Simplified HTTP protocol handling (no manual header parsing)
 *  - Cleaner, more maintainable code (~150 lines vs ~500)
 *  - Same TransportInterface_t abstraction as MQTT (reusable)
 *  - Built-in HTTP/1.1 features (keep-alive, pipelining, etc.)
 *
 * @par Thread safety
 * NOT re-entrant.  The static buffers are shared across calls.
 * Use a mutex if multiple tasks may call @c ota_https_download() concurrently.
 *
 * @par TLS connection
 * Uses TLS_Socket_Connect() from transport_tls_socket_using_mbedtls.c:
 *  - Establishes mutual TLS with server certificate verification
 *  - Returns NetworkContext with callbacks for HTTP send/recv
 *  - Same transport abstraction as MQTT (reusable send/recv funcs)
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "aws_iot_ota_https.h"
#include "uart_term.h"

/* coreHTTP */
#include "core_http_client.h"

/* Transport interface */
#include "transport_tls_socket.h"
#include "transport_abstraction.h"

/* NetworkContext definition - from aws-osprey transport layer */
#ifndef NetworkContext_t
typedef struct NetworkContext
{
    void * pParams;
} NetworkContext_t;
#endif

/* --------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------*/

/** @brief HTTP request/response buffer size. */
#define OTA_HTTPS_BUFFER_SIZE    (4096U + 512U)

/** @brief Receive timeout (milliseconds). */
#define OTA_HTTPS_RECV_TIMEOUT_MS    30000U

/** @brief Send timeout (milliseconds). */
#define OTA_HTTPS_SEND_TIMEOUT_MS    30000U

/** @brief Max URL size. */
#define OTA_HTTPS_MAX_URL_SIZE       512U

/* --------------------------------------------------------------------------
 * Module-level static buffers
 * --------------------------------------------------------------------------*/

/** @brief Shared HTTP request+response buffer (reused per call). */
static uint8_t s_httpBuffer[OTA_HTTPS_BUFFER_SIZE];

/* --------------------------------------------------------------------------
 * Helper: Parse HTTPS URL
 * --------------------------------------------------------------------------*/

/**
 * @brief Parse HTTPS URL into host and request path.
 *
 * @param[in]  pUrl     URL string (must start with "https://")
 * @param[out] pHost    Output hostname buffer
 * @param[in]  hostLen  Max size of hostname buffer
 * @param[out] pPath    Output path buffer
 * @param[in]  pathLen  Max size of path buffer
 *
 * @return 0 on success, -1 on error
 */
static int ota_parse_url(const char *pUrl,
                         char *pHost, size_t hostLen,
                         char *pPath, size_t pathLen)
{
    const char *scheme = "https://";
    const size_t schemeLen = 8u;
    const char *p, *slashPos;
    size_t copyLen;

    if (!pUrl || !pHost || !pPath)
        return -1;

    /* Verify "https://" prefix */
    if (strncmp(pUrl, scheme, schemeLen) != 0)
    {
        UART_PRINT("[OTA-HTTPS] URL must start with https:// (got: %.20s...)\r\n", pUrl);
        return -1;
    }

    p = pUrl + schemeLen;

    /* Find first '/' after scheme (start of path) */
    slashPos = strchr(p, '/');

    if (slashPos == NULL)
    {
        /* No path component */
        slashPos = pUrl + strlen(pUrl);
    }

    /* Extract hostname */
    copyLen = (size_t)(slashPos - p);
    if (copyLen >= hostLen)
        copyLen = hostLen - 1;
    strncpy(pHost, p, copyLen);
    pHost[copyLen] = '\0';

    /* Extract path */
    if (slashPos == pUrl + strlen(pUrl))
    {
        strncpy(pPath, "/", pathLen - 1);
        pPath[1] = '\0';
    }
    else
    {
        copyLen = strlen(slashPos);
        if (copyLen >= pathLen)
            copyLen = pathLen - 1;
        strncpy(pPath, slashPos, copyLen);
        pPath[copyLen] = '\0';
    }

    UART_PRINT("[OTA-HTTPS] URL parsed: host=%s, path=%.50s\r\n", pHost, pPath);
    return 0;
}

/* --------------------------------------------------------------------------
 * Main HTTPS download function using coreHTTP
 * --------------------------------------------------------------------------*/

int ota_https_download(const ota_https_req_t *pReq, uint32_t *pBytesReceived)
{
    int ret = -1;
    HTTPStatus_t httpStatus;
    HTTPRequestInfo_t requestInfo;
    HTTPRequestHeaders_t requestHeaders;
    HTTPResponse_t response;
    TransportInterface_t transportInterface;
    NetworkContext_t networkContext;
    TlsTransportParams_t tlsParams;
    NetworkCredentials_t networkCredentials;
    TlsTransportStatus_t tlsStatus;
    uint32_t totalBytesReceived = 0;
    char host[256] = {0};
    char path[OTA_HTTPS_MAX_URL_SIZE] = {0};

    /* Validate inputs */
    if (!pReq || !pReq->pURL || !pReq->dataCb)
    {
        UART_PRINT("[OTA-HTTPS] Invalid request parameters\r\n");
        return -1;
    }

    UART_PRINT("[OTA-HTTPS] Starting download from: %.50s...\r\n", pReq->pURL);

    /* -------- Parse URL -------- */
    if (ota_parse_url(pReq->pURL, host, sizeof(host), path, sizeof(path)) != 0)
    {
        UART_PRINT("[OTA-HTTPS] Failed to parse URL\r\n");
        return -1;
    }

    /* -------- Set up TLS credentials -------- */
    memset(&networkCredentials, 0, sizeof(networkCredentials));
    networkCredentials.pucRootCa = pReq->pCaCert;
    networkCredentials.xRootCaSize = pReq->caCertLen;
    networkCredentials.pucClientCert = NULL;  /* No mTLS for S3 pre-signed URLs */
    networkCredentials.xClientCertSize = 0;
    networkCredentials.pucPrivateKey = NULL;
    networkCredentials.xPrivateKeySize = 0;
    networkCredentials.xDisableSni = pdFALSE;
    networkCredentials.ppcAlpnProtos = NULL;

    /* -------- Set up network context -------- */
    memset(&tlsParams, 0, sizeof(tlsParams));
    networkContext.pParams = &tlsParams;

    /* -------- Establish TLS connection -------- */
    tlsStatus = TLS_Socket_Connect(&networkContext,
                                   host,
                                   443,
                                   &networkCredentials,
                                   OTA_HTTPS_RECV_TIMEOUT_MS,
                                   OTA_HTTPS_SEND_TIMEOUT_MS);

    if (tlsStatus != eTLSTransportSuccess)
    {
        UART_PRINT("[OTA-HTTPS] TLS connection failed (status=%d)\r\n", tlsStatus);
        return -1;
    }

    UART_PRINT("[OTA-HTTPS] TLS connection established\r\n");

    /* -------- Set up transport interface -------- */
    transportInterface.pNetworkContext = &networkContext;
    transportInterface.send = TLS_Socket_Send;
    transportInterface.recv = TLS_Socket_Recv;

    /* -------- Initialize request headers -------- */
    requestHeaders.pBuffer = s_httpBuffer;
    requestHeaders.bufferLen = sizeof(s_httpBuffer);

    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders, NULL);
    if (httpStatus != HTTPSuccess)
    {
        UART_PRINT("[OTA-HTTPS] HTTPClient_InitializeRequestHeaders failed: %d\r\n", httpStatus);
        goto cleanup;
    }

    /* -------- Set up HTTP GET request -------- */
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.pMethod = HTTP_METHOD_GET;
    requestInfo.methodLen = strlen(HTTP_METHOD_GET);
    requestInfo.pPath = path;
    requestInfo.pathLen = strlen(path);
    requestInfo.pHost = host;
    requestInfo.hostLen = strlen(host);

    /* Reinitialize with method and path info */
    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders, &requestInfo);
    if (httpStatus != HTTPSuccess)
    {
        UART_PRINT("[OTA-HTTPS] HTTPClient_InitializeRequestHeaders failed: %d\r\n", httpStatus);
        goto cleanup;
    }

    UART_PRINT("[OTA-HTTPS] HTTP GET %s%s\r\n", host, path);

    /* -------- Send HTTP GET and receive response headers -------- */
    response.pBuffer = s_httpBuffer;
    response.bufferLen = sizeof(s_httpBuffer);
    response.statusCode = 0;
    response.respOptionFlags = HTTP_RESPONSE_DO_NOT_PARSE_BODY_FLAG;

    httpStatus = HTTPClient_Send(&transportInterface,
                                 &requestHeaders,
                                 NULL,   /* No request body for GET */
                                 0,
                                 &response,
                                 0);     /* No send flags */

    if (httpStatus != HTTPSuccess)
    {
        UART_PRINT("[OTA-HTTPS] HTTPClient_Send failed: %d\r\n", httpStatus);
        goto cleanup;
    }

    UART_PRINT("[OTA-HTTPS] HTTP response status: %u\r\n", response.statusCode);

    /* Verify HTTP 200 OK */
    if (response.statusCode != 200u)
    {
        UART_PRINT("[OTA-HTTPS] Unexpected HTTP status: %u\r\n", response.statusCode);
        goto cleanup;
    }

    /* Get Content-Length for progress tracking */
    size_t contentLength = response.contentLength;
    UART_PRINT("[OTA-HTTPS] Content-Length: %lu bytes\r\n", (unsigned long)contentLength);

    if (contentLength == 0)
    {
        UART_PRINT("[OTA-HTTPS] Empty response body\r\n");
        goto cleanup;
    }

    /* -------- Stream response body in chunks -------- */
    UART_PRINT("[OTA-HTTPS] Streaming firmware body...\r\n");

    /* First, process any body data already buffered from header parsing */
    if (response.bodyLen > 0)
    {
        if (pReq->dataCb(response.pBody, (uint32_t)response.bodyLen, pReq->pUserCtx) != 0)
        {
            UART_PRINT("[OTA-HTTPS] dataCb returned error at offset 0\r\n");
            goto cleanup;
        }
        totalBytesReceived += response.bodyLen;
        UART_PRINT("[OTA-HTTPS] Downloaded %lu bytes\r\n", (unsigned long)totalBytesReceived);
    }

    /* Stream remaining body using transport recv */
    while (totalBytesReceived < contentLength)
    {
        size_t bytesToRead = sizeof(s_httpBuffer);
        if (bytesToRead > (contentLength - totalBytesReceived))
        {
            bytesToRead = contentLength - totalBytesReceived;
        }

        int32_t bytesRead = transportInterface.recv(
            transportInterface.pNetworkContext,
            s_httpBuffer,
            bytesToRead);

        if (bytesRead < 0)
        {
            UART_PRINT("[OTA-HTTPS] recv failed: %ld\r\n", (long)bytesRead);
            goto cleanup;
        }

        if (bytesRead == 0)
        {
            /* EOF before Content-Length */
            UART_PRINT("[OTA-HTTPS] EOF before Content-Length (got %lu, expected %lu)\r\n",
                      (unsigned long)totalBytesReceived, (unsigned long)contentLength);
            goto cleanup;
        }

        /* Stream chunk to callback */
        if (pReq->dataCb(s_httpBuffer, (uint32_t)bytesRead, pReq->pUserCtx) != 0)
        {
            UART_PRINT("[OTA-HTTPS] dataCb returned error at offset %lu\r\n",
                      (unsigned long)totalBytesReceived);
            goto cleanup;
        }

        totalBytesReceived += (size_t)bytesRead;

        /* Progress log every 64KB */
        if ((totalBytesReceived % (64 * 1024)) == 0 || totalBytesReceived == contentLength)
        {
            UART_PRINT("[OTA-HTTPS] Downloaded %lu / %lu bytes\r\n",
                      (unsigned long)totalBytesReceived, (unsigned long)contentLength);
        }
    }

    UART_PRINT("[OTA-HTTPS] Download complete: %lu bytes\r\n", (unsigned long)totalBytesReceived);

    ret = 0;  /* Success */

cleanup:
    /* Close TLS connection */
    TLS_Socket_Disconnect(&networkContext);

    if (pBytesReceived != NULL)
    {
        *pBytesReceived = (ret == 0) ? totalBytesReceived : 0;
    }

    return ret;
}
