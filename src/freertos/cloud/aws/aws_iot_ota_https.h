/**
 * @file aws_iot_ota_https.h
 * @brief HTTPS firmware downloader for OTA updates on CC35X1.
 *
 * Streams the body of an HTTPS GET response to a caller-supplied callback,
 * one chunk at a time.  Intended for downloading firmware images from
 * pre-signed S3 URLs produced by AWS IoT OTA jobs.
 *
 * TLS is provided by mbedTLS (@c config-hsm.h / PSA) over a LwIP BSD socket.
 * No mutual TLS is used — S3 pre-signed URLs authenticate via query parameters.
 *
 * @par Thread safety
 * @c ota_https_download() uses static module-level mbedTLS buffers and is
 * therefore NOT re-entrant.  Do not call it from more than one task
 * concurrently.
 */

#ifndef AWS_IOT_OTA_HTTPS_H
#define AWS_IOT_OTA_HTTPS_H

#ifndef MBEDTLS_CONFIG_FILE
#define MBEDTLS_CONFIG_FILE "config-hsm.h"
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * --------------------------------------------------------------------------*/

/**
 * @brief Callback invoked for each chunk of HTTP response body data.
 *
 * @param[in] pData   Pointer to the received data chunk.  Valid only for the
 *                    duration of the callback.
 * @param[in] len     Number of bytes in @p pData.
 * @param[in] pCtx    Caller-supplied context pointer (from @c ota_https_req_t).
 *
 * @return @c 0 to continue downloading.
 * @return Non-zero to abort the download (causes @c ota_https_download to
 *         return a negative error code).
 */
typedef int (*ota_https_data_cb_t)(const uint8_t *pData, uint32_t len,
                                   void *pCtx);

/**
 * @brief Parameters for a single HTTPS GET download request.
 *
 * All pointer fields must remain valid until @c ota_https_download() returns.
 */
typedef struct
{
    const char          *pURL;      /**< Full HTTPS URL, e.g. a pre-signed S3 URL.
                                      *   Must begin with "https://".  Must not be NULL. */
    const uint8_t       *pCaCert;   /**< PEM-encoded CA certificate, null-terminated.
                                      *   Pass NULL to skip server certificate verification
                                      *   (not recommended for production). */
    uint32_t             caCertLen; /**< Byte length of @c pCaCert including the null
                                      *   terminator.  Ignored when @c pCaCert is NULL. */
    ota_https_data_cb_t  dataCb;    /**< Called once per received chunk of body data.
                                      *   Must not be NULL. */
    void                *pUserCtx;  /**< Opaque pointer passed to @c dataCb unchanged. */
} ota_https_req_t;

/* --------------------------------------------------------------------------
 * Functions
 * --------------------------------------------------------------------------*/

/**
 * @brief Download a file via HTTPS GET and stream the body to a callback.
 *
 * Execution sequence:
 * -# Parses the URL into host and path components.
 * -# Resolves the host via @c lwip_getaddrinfo (IPv4 only).
 * -# Opens a TCP socket and connects to port 443.
 * -# Performs a TLS handshake (server-only verification when @c pCaCert is
 *    provided; @c VERIFY_NONE otherwise).
 * -# Sends an HTTP/1.1 GET request.
 * -# Reads and parses the response headers; returns an error if the HTTP
 *    status code is not 200.
 * -# Calls @c pReq->dataCb for each body chunk until:
 *    - The TLS connection is closed by the peer, OR
 *    - The number of bytes received equals the Content-Length value, OR
 *    - @c dataCb returns non-zero.
 * -# Closes the TLS session and socket unconditionally.
 *
 * @param[in]  pReq            Download request parameters.  Must not be NULL.
 * @param[out] pBytesReceived  Set to the total number of body bytes delivered
 *                             to @c dataCb.  Set to 0 on error.  May be NULL
 *                             if the caller does not need this value.
 *
 * @return @c 0 on success.
 * @return A negative value on any error (DNS failure, TLS handshake failure,
 *         non-200 HTTP status, socket error, or @c dataCb returning non-zero).
 */
int ota_https_download(const ota_https_req_t *pReq, uint32_t *pBytesReceived);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_OTA_HTTPS_H */
