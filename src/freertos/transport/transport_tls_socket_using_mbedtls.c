/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file transport_tls_socket_using_mbedtls.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <string.h>

/* Include header that defines log levels. */
#include "logging/logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "TlsTransport"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif


/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
//extern void vLoggingPrintf( const char * pcFormatString,
//                            ... );
#include "uart_term.h"
/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
//#ifndef SdkLog
//    #define SdkLog( message )    UART_PRINT message
//#endif

#include "logging/logging_stack.h"

/************ End of logging configuration ****************/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* TLS transport header. */
#include "transport_tls_socket.h"

/* FreeRTOS Socket wrapper include. */
#include "sockets_wrapper.h"

/* mbedTLS util includes. */
#include "mbedtls/net_sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "entropy_poll.h"
#include "mbedtls/ssl.h"
#include "mbedtls/threading.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"

#include "threading_alt.h"

#include "platform.h"

/* mbed TLS includes. */
#include "../configs/mbedtls_config.h"


/*-----------------------------------------------------------*/

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's TlsTransportParams_t) */
struct NetworkContext
{
    /* TlsTransportParams_t */
    void * pParams;
};

/**
 * @brief Secured connection context.
 */
typedef struct MbedSSLContext
{
    mbedtls_ssl_config config;               /**< @brief SSL connection configuration. */
    mbedtls_ssl_context context;             /**< @brief SSL connection context */
    mbedtls_x509_crt_profile certProfile;    /**< @brief Certificate security profile for this connection. */
    mbedtls_x509_crt rootCa;                 /**< @brief Root CA certificate context. */
    mbedtls_x509_crt clientCert;             /**< @brief Client certificate context. */
    mbedtls_pk_context privKey;              /**< @brief Client private key context. */
    mbedtls_entropy_context entropyContext;  /**< @brief Entropy context for random number generation. */
    mbedtls_ctr_drbg_context ctrDrgbContext; /**< @brief CTR DRBG context for random number generation. */
} MbedSSLContext_t;

/*-----------------------------------------------------------*/

/**
 * @brief Sends data over FreeRTOS+TCP sockets.
 *
 * @param[in] ctx The network context containing the socket handle.
 * @param[in] buf Buffer containing the bytes to send.
 * @param[in] len Number of bytes to send from the buffer.
 *
 * @return Number of bytes sent on success; else a negative value.
 */
int mbedtls_platform_send( void * ctx,
                           const unsigned char * buf,
                           size_t len )
{
    SocketHandle socket;

    configASSERT( buf != NULL );

    socket = ( SocketHandle ) ctx;

    return ( int ) Sockets_Send(socket, buf, len );

}

/*-----------------------------------------------------------*/

/**
 * @brief Receives data from FreeRTOS+TCP socket.
 *
 * @param[in] ctx The network context containing the socket handle.
 * @param[out] buf Buffer to receive bytes into.
 * @param[in] len Number of bytes to receive from the network.
 *
 * @return Number of bytes received if successful; Negative value on error.
 */
int mbedtls_platform_recv( void * ctx,
                           unsigned char * buf,
                           size_t len )
{
    SocketHandle socket;

    configASSERT( buf != NULL );

    socket = ( SocketHandle ) ctx;

    return ( int ) Sockets_Recv( socket, buf, len );

}


#include "mbedtls/platform.h"

#include <stdio.h>
#include <string.h>

#if defined(MBEDTLS_AES_C)
#include "mbedtls/aes.h"
#endif

#if defined(MBEDTLS_ARIA_C)
#include "mbedtls/aria.h"
#endif

#if defined(MBEDTLS_ASN1_PARSE_C)
#include "mbedtls/asn1.h"
#endif

#if defined(MBEDTLS_BASE64_C)
#include "mbedtls/base64.h"
#endif

#if defined(MBEDTLS_BIGNUM_C)
#include "mbedtls/bignum.h"
#endif

#if defined(MBEDTLS_CAMELLIA_C)
#include "mbedtls/camellia.h"
#endif

#if defined(MBEDTLS_CCM_C)
#include "mbedtls/ccm.h"
#endif

#if defined(MBEDTLS_CHACHA20_C)
#include "mbedtls/chacha20.h"
#endif

#if defined(MBEDTLS_CHACHAPOLY_C)
#include "mbedtls/chachapoly.h"
#endif

#if defined(MBEDTLS_CIPHER_C)
#include "mbedtls/cipher.h"
#endif

#if defined(MBEDTLS_CTR_DRBG_C)
#include "mbedtls/ctr_drbg.h"
#endif

#if defined(MBEDTLS_DES_C)
#include "mbedtls/des.h"
#endif

#if defined(MBEDTLS_DHM_C)
#include "mbedtls/dhm.h"
#endif

#if defined(MBEDTLS_ECP_C)
#include "mbedtls/ecp.h"
#endif

#if defined(MBEDTLS_ENTROPY_C)
#include "mbedtls/entropy.h"
#endif

#if defined(MBEDTLS_ERROR_C)
#include "mbedtls/error.h"
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#endif

#if defined(MBEDTLS_GCM_C)
#include "mbedtls/gcm.h"
#endif

#if defined(MBEDTLS_HKDF_C)
#include "mbedtls/hkdf.h"
#endif

#if defined(MBEDTLS_HMAC_DRBG_C)
#include "mbedtls/hmac_drbg.h"
#endif

#if defined(MBEDTLS_LMS_C)
#include "mbedtls/lms.h"
#endif

#if defined(MBEDTLS_MD_C)
#include "mbedtls/md.h"
#endif

#if defined(MBEDTLS_NET_C)
#include "mbedtls/net_sockets.h"
#endif

#if defined(MBEDTLS_OID_C)
#include "mbedtls/oid.h"
#endif

#if defined(MBEDTLS_PEM_PARSE_C) || defined(MBEDTLS_PEM_WRITE_C)
#include "mbedtls/pem.h"
#endif

#if defined(MBEDTLS_PK_C)
#include "mbedtls/pk.h"
#endif

#if defined(MBEDTLS_PKCS12_C)
#include "mbedtls/pkcs12.h"
#endif

#if defined(MBEDTLS_PKCS5_C)
#include "mbedtls/pkcs5.h"
#endif

#if defined(MBEDTLS_PKCS7_C)
#include "mbedtls/pkcs7.h"
#endif

#if defined(MBEDTLS_POLY1305_C)
#include "mbedtls/poly1305.h"
#endif

#if defined(MBEDTLS_RSA_C)
#include "mbedtls/rsa.h"
#endif

#if defined(MBEDTLS_SHA1_C)
#include "mbedtls/sha1.h"
#endif

#if defined(MBEDTLS_SHA256_C)
#include "mbedtls/sha256.h"
#endif

#if defined(MBEDTLS_SHA3_C)
#include "mbedtls/sha3.h"
#endif

#if defined(MBEDTLS_SHA512_C)
#include "mbedtls/sha512.h"
#endif

#if defined(MBEDTLS_SSL_TLS_C)
#include "mbedtls/ssl.h"
#endif

#if defined(MBEDTLS_THREADING_C)
#include "mbedtls/threading.h"
#endif

#if defined(MBEDTLS_X509_USE_C) || defined(MBEDTLS_X509_CREATE_C)
#include "mbedtls/x509.h"
#endif

// const char *mbedtls_high_level_strerr(int error_code)
// {
//     int high_level_error_code;

//     if (error_code < 0) {
//         error_code = -error_code;
//     }

//     /* Extract the high-level part from the error code. */
//     high_level_error_code = error_code & 0xFF80;

//     switch (high_level_error_code) {
//     /* Begin Auto-Generated Code. */
//     #if defined(MBEDTLS_CIPHER_C)
//         case -(MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE):
//             return( "CIPHER - The selected feature is not available" );
//         case -(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA):
//             return( "CIPHER - Bad input parameters" );
//         case -(MBEDTLS_ERR_CIPHER_ALLOC_FAILED):
//             return( "CIPHER - Failed to allocate memory" );
//         case -(MBEDTLS_ERR_CIPHER_INVALID_PADDING):
//             return( "CIPHER - Input data contains invalid padding and is rejected" );
//         case -(MBEDTLS_ERR_CIPHER_FULL_BLOCK_EXPECTED):
//             return( "CIPHER - Decryption of block requires a full block" );
//         case -(MBEDTLS_ERR_CIPHER_AUTH_FAILED):
//             return( "CIPHER - Authentication failed (for AEAD modes)" );
//         case -(MBEDTLS_ERR_CIPHER_INVALID_CONTEXT):
//             return( "CIPHER - The context is invalid. For example, because it was freed" );
// #endif /* MBEDTLS_CIPHER_C */

// #if defined(MBEDTLS_DHM_C)
//         case -(MBEDTLS_ERR_DHM_BAD_INPUT_DATA):
//             return( "DHM - Bad input parameters" );
//         case -(MBEDTLS_ERR_DHM_READ_PARAMS_FAILED):
//             return( "DHM - Reading of the DHM parameters failed" );
//         case -(MBEDTLS_ERR_DHM_MAKE_PARAMS_FAILED):
//             return( "DHM - Making of the DHM parameters failed" );
//         case -(MBEDTLS_ERR_DHM_READ_PUBLIC_FAILED):
//             return( "DHM - Reading of the public values failed" );
//         case -(MBEDTLS_ERR_DHM_MAKE_PUBLIC_FAILED):
//             return( "DHM - Making of the public value failed" );
//         case -(MBEDTLS_ERR_DHM_CALC_SECRET_FAILED):
//             return( "DHM - Calculation of the DHM secret failed" );
//         case -(MBEDTLS_ERR_DHM_INVALID_FORMAT):
//             return( "DHM - The ASN.1 data is not formatted correctly" );
//         case -(MBEDTLS_ERR_DHM_ALLOC_FAILED):
//             return( "DHM - Allocation of memory failed" );
//         case -(MBEDTLS_ERR_DHM_FILE_IO_ERROR):
//             return( "DHM - Read or write of file failed" );
//         case -(MBEDTLS_ERR_DHM_SET_GROUP_FAILED):
//             return( "DHM - Setting the modulus and generator failed" );
// #endif /* MBEDTLS_DHM_C */

// #if defined(MBEDTLS_ECP_C)
//         case -(MBEDTLS_ERR_ECP_BAD_INPUT_DATA):
//             return( "ECP - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL):
//             return( "ECP - The buffer is too small to write to" );
//         case -(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE):
//             return( "ECP - The requested feature is not available, for example, the requested curve is not supported" );
//         case -(MBEDTLS_ERR_ECP_VERIFY_FAILED):
//             return( "ECP - The signature is not valid" );
//         case -(MBEDTLS_ERR_ECP_ALLOC_FAILED):
//             return( "ECP - Memory allocation failed" );
//         case -(MBEDTLS_ERR_ECP_RANDOM_FAILED):
//             return( "ECP - Generation of random value, such as ephemeral key, failed" );
//         case -(MBEDTLS_ERR_ECP_INVALID_KEY):
//             return( "ECP - Invalid private or public key" );
//         case -(MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH):
//             return( "ECP - The buffer contains a valid signature followed by more data" );
//         case -(MBEDTLS_ERR_ECP_IN_PROGRESS):
//             return( "ECP - Operation in progress, call again with the same parameters to continue" );
// #endif /* MBEDTLS_ECP_C */

// #if defined(MBEDTLS_MD_C)
//         case -(MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE):
//             return( "MD - The selected feature is not available" );
//         case -(MBEDTLS_ERR_MD_BAD_INPUT_DATA):
//             return( "MD - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_MD_ALLOC_FAILED):
//             return( "MD - Failed to allocate memory" );
//         case -(MBEDTLS_ERR_MD_FILE_IO_ERROR):
//             return( "MD - Opening or reading of file failed" );
// #endif /* MBEDTLS_MD_C */

// #if defined(MBEDTLS_PEM_PARSE_C) || defined(MBEDTLS_PEM_WRITE_C)
//         case -(MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT):
//             return( "PEM - No PEM header or footer found" );
//         case -(MBEDTLS_ERR_PEM_INVALID_DATA):
//             return( "PEM - PEM string is not as expected" );
//         case -(MBEDTLS_ERR_PEM_ALLOC_FAILED):
//             return( "PEM - Failed to allocate memory" );
//         case -(MBEDTLS_ERR_PEM_INVALID_ENC_IV):
//             return( "PEM - RSA IV is not in hex-format" );
//         case -(MBEDTLS_ERR_PEM_UNKNOWN_ENC_ALG):
//             return( "PEM - Unsupported key encryption algorithm" );
//         case -(MBEDTLS_ERR_PEM_PASSWORD_REQUIRED):
//             return( "PEM - Private key password can't be empty" );
//         case -(MBEDTLS_ERR_PEM_PASSWORD_MISMATCH):
//             return( "PEM - Given private key password does not allow for correct decryption" );
//         case -(MBEDTLS_ERR_PEM_FEATURE_UNAVAILABLE):
//             return( "PEM - Unavailable feature, e.g. hashing/encryption combination" );
//         case -(MBEDTLS_ERR_PEM_BAD_INPUT_DATA):
//             return( "PEM - Bad input parameters to function" );
// #endif /* MBEDTLS_PEM_PARSE_C || MBEDTLS_PEM_WRITE_C */

// #if defined(MBEDTLS_PK_C)
//         case -(MBEDTLS_ERR_PK_ALLOC_FAILED):
//             return( "PK - Memory allocation failed" );
//         case -(MBEDTLS_ERR_PK_TYPE_MISMATCH):
//             return( "PK - Type mismatch, eg attempt to encrypt with an ECDSA key" );
//         case -(MBEDTLS_ERR_PK_BAD_INPUT_DATA):
//             return( "PK - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_PK_FILE_IO_ERROR):
//             return( "PK - Read/write of file failed" );
//         case -(MBEDTLS_ERR_PK_KEY_INVALID_VERSION):
//             return( "PK - Unsupported key version" );
//         case -(MBEDTLS_ERR_PK_KEY_INVALID_FORMAT):
//             return( "PK - Invalid key tag or value" );
//         case -(MBEDTLS_ERR_PK_UNKNOWN_PK_ALG):
//             return( "PK - Key algorithm is unsupported (only RSA and EC are supported)" );
//         case -(MBEDTLS_ERR_PK_PASSWORD_REQUIRED):
//             return( "PK - Private key password can't be empty" );
//         case -(MBEDTLS_ERR_PK_PASSWORD_MISMATCH):
//             return( "PK - Given private key password does not allow for correct decryption" );
//         case -(MBEDTLS_ERR_PK_INVALID_PUBKEY):
//             return( "PK - The pubkey tag or value is invalid (only RSA and EC are supported)" );
//         case -(MBEDTLS_ERR_PK_INVALID_ALG):
//             return( "PK - The algorithm tag or value is invalid" );
//         case -(MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE):
//             return( "PK - Elliptic curve is unsupported (only NIST curves are supported)" );
//         case -(MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE):
//             return( "PK - Unavailable feature, e.g. RSA disabled for RSA key" );
//         case -(MBEDTLS_ERR_PK_SIG_LEN_MISMATCH):
//             return( "PK - The buffer contains a valid signature followed by more data" );
//         case -(MBEDTLS_ERR_PK_BUFFER_TOO_SMALL):
//             return( "PK - The output buffer is too small" );
// #endif /* MBEDTLS_PK_C */

// #if defined(MBEDTLS_PKCS12_C)
//         case -(MBEDTLS_ERR_PKCS12_BAD_INPUT_DATA):
//             return( "PKCS12 - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_PKCS12_FEATURE_UNAVAILABLE):
//             return( "PKCS12 - Feature not available, e.g. unsupported encryption scheme" );
//         case -(MBEDTLS_ERR_PKCS12_PBE_INVALID_FORMAT):
//             return( "PKCS12 - PBE ASN.1 data not as expected" );
//         case -(MBEDTLS_ERR_PKCS12_PASSWORD_MISMATCH):
//             return( "PKCS12 - Given private key password does not allow for correct decryption" );
// #endif /* MBEDTLS_PKCS12_C */

// #if defined(MBEDTLS_PKCS5_C)
//         case -(MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA):
//             return( "PKCS5 - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_PKCS5_INVALID_FORMAT):
//             return( "PKCS5 - Unexpected ASN.1 data" );
//         case -(MBEDTLS_ERR_PKCS5_FEATURE_UNAVAILABLE):
//             return( "PKCS5 - Requested encryption or digest alg not available" );
//         case -(MBEDTLS_ERR_PKCS5_PASSWORD_MISMATCH):
//             return( "PKCS5 - Given private key password does not allow for correct decryption" );
// #endif /* MBEDTLS_PKCS5_C */

// #if defined(MBEDTLS_PKCS7_C)
//         case -(MBEDTLS_ERR_PKCS7_INVALID_FORMAT):
//             return( "PKCS7 - The format is invalid, e.g. different type expected" );
//         case -(MBEDTLS_ERR_PKCS7_FEATURE_UNAVAILABLE):
//             return( "PKCS7 - Unavailable feature, e.g. anything other than signed data" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_VERSION):
//             return( "PKCS7 - The PKCS #7 version element is invalid or cannot be parsed" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_CONTENT_INFO):
//             return( "PKCS7 - The PKCS #7 content info is invalid or cannot be parsed" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_ALG):
//             return( "PKCS7 - The algorithm tag or value is invalid or cannot be parsed" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_CERT):
//             return( "PKCS7 - The certificate tag or value is invalid or cannot be parsed" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_SIGNATURE):
//             return( "PKCS7 - Error parsing the signature" );
//         case -(MBEDTLS_ERR_PKCS7_INVALID_SIGNER_INFO):
//             return( "PKCS7 - Error parsing the signer's info" );
//         case -(MBEDTLS_ERR_PKCS7_BAD_INPUT_DATA):
//             return( "PKCS7 - Input invalid" );
//         case -(MBEDTLS_ERR_PKCS7_ALLOC_FAILED):
//             return( "PKCS7 - Allocation of memory failed" );
//         case -(MBEDTLS_ERR_PKCS7_VERIFY_FAIL):
//             return( "PKCS7 - Verification Failed" );
//         case -(MBEDTLS_ERR_PKCS7_CERT_DATE_INVALID):
//             return( "PKCS7 - The PKCS #7 date issued/expired dates are invalid" );
// #endif /* MBEDTLS_PKCS7_C */

// #if defined(MBEDTLS_RSA_C)
//         case -(MBEDTLS_ERR_RSA_BAD_INPUT_DATA):
//             return( "RSA - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_RSA_INVALID_PADDING):
//             return( "RSA - Input data contains invalid padding and is rejected" );
//         case -(MBEDTLS_ERR_RSA_KEY_GEN_FAILED):
//             return( "RSA - Something failed during generation of a key" );
//         case -(MBEDTLS_ERR_RSA_KEY_CHECK_FAILED):
//             return( "RSA - Key failed to pass the validity check of the library" );
//         case -(MBEDTLS_ERR_RSA_PUBLIC_FAILED):
//             return( "RSA - The public key operation failed" );
//         case -(MBEDTLS_ERR_RSA_PRIVATE_FAILED):
//             return( "RSA - The private key operation failed" );
//         case -(MBEDTLS_ERR_RSA_VERIFY_FAILED):
//             return( "RSA - The PKCS#1 verification failed" );
//         case -(MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE):
//             return( "RSA - The output buffer for decryption is not large enough" );
//         case -(MBEDTLS_ERR_RSA_RNG_FAILED):
//             return( "RSA - The random generator failed to generate non-zeros" );
// #endif /* MBEDTLS_RSA_C */

// #if defined(MBEDTLS_SSL_TLS_C)
//         case -(MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS):
//             return( "SSL - A cryptographic operation is in progress. Try again later" );
//         case -(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE):
//             return( "SSL - The requested feature is not available" );
//         case -(MBEDTLS_ERR_SSL_BAD_INPUT_DATA):
//             return( "SSL - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_SSL_INVALID_MAC):
//             return( "SSL - Verification of the message MAC failed" );
//         case -(MBEDTLS_ERR_SSL_INVALID_RECORD):
//             return( "SSL - An invalid SSL record was received" );
//         case -(MBEDTLS_ERR_SSL_CONN_EOF):
//             return( "SSL - The connection indicated an EOF" );
//         case -(MBEDTLS_ERR_SSL_DECODE_ERROR):
//             return( "SSL - A message could not be parsed due to a syntactic error" );
//         case -(MBEDTLS_ERR_SSL_NO_RNG):
//             return( "SSL - No RNG was provided to the SSL module" );
//         case -(MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE):
//             return( "SSL - No client certification received from the client, but required by the authentication mode" );
//         case -(MBEDTLS_ERR_SSL_UNSUPPORTED_EXTENSION):
//             return( "SSL - Client received an extended server hello containing an unsupported extension" );
//         case -(MBEDTLS_ERR_SSL_NO_APPLICATION_PROTOCOL):
//             return( "SSL - No ALPN protocols supported that the client advertises" );
//         case -(MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED):
//             return( "SSL - The own private key or pre-shared key is not set, but needed" );
//         case -(MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED):
//             return( "SSL - No CA Chain is set, but required to operate" );
//         case -(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE):
//             return( "SSL - An unexpected message was received from our peer" );
//         case -(MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE):
//             return( "SSL - A fatal alert message was received from our peer" );
//         case -(MBEDTLS_ERR_SSL_UNRECOGNIZED_NAME):
//             return( "SSL - No server could be identified matching the client's SNI" );
//         case -(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY):
//             return( "SSL - The peer notified us that the connection is going to be closed" );
//         case -(MBEDTLS_ERR_SSL_BAD_CERTIFICATE):
//             return( "SSL - Processing of the Certificate handshake message failed" );
//         case -(MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET):
//             return( "SSL - * Received NewSessionTicket Post Handshake Message. This error code is experimental and may be changed or removed without notice" );
//         case -(MBEDTLS_ERR_SSL_CANNOT_READ_EARLY_DATA):
//             return( "SSL - Not possible to read early data" );
//         case -(MBEDTLS_ERR_SSL_CANNOT_WRITE_EARLY_DATA):
//             return( "SSL - Not possible to write early data" );
//         case -(MBEDTLS_ERR_SSL_CACHE_ENTRY_NOT_FOUND):
//             return( "SSL - Cache entry not found" );
//         case -(MBEDTLS_ERR_SSL_ALLOC_FAILED):
//             return( "SSL - Memory allocation failed" );
//         case -(MBEDTLS_ERR_SSL_HW_ACCEL_FAILED):
//             return( "SSL - Hardware acceleration function returned with error" );
//         case -(MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH):
//             return( "SSL - Hardware acceleration function skipped / left alone data" );
//         case -(MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION):
//             return( "SSL - Handshake protocol not within min/max boundaries" );
//         case -(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE):
//             return( "SSL - The handshake negotiation failed" );
//         case -(MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED):
//             return( "SSL - Session ticket has expired" );
//         case -(MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH):
//             return( "SSL - Public key type mismatch (eg, asked for RSA key exchange and presented EC key)" );
//         case -(MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY):
//             return( "SSL - Unknown identity received (eg, PSK identity)" );
//         case -(MBEDTLS_ERR_SSL_INTERNAL_ERROR):
//             return( "SSL - Internal error (eg, unexpected failure in lower-level module)" );
//         case -(MBEDTLS_ERR_SSL_COUNTER_WRAPPING):
//             return( "SSL - A counter would wrap (eg, too many messages exchanged)" );
//         case -(MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO):
//             return( "SSL - Unexpected message at ServerHello in renegotiation" );
//         case -(MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED):
//             return( "SSL - DTLS client must retry for hello verification" );
//         case -(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL):
//             return( "SSL - A buffer is too small to receive or write a message" );
//         case -(MBEDTLS_ERR_SSL_WANT_READ):
//             return( "SSL - No data of requested type currently available on underlying transport" );
//         case -(MBEDTLS_ERR_SSL_WANT_WRITE):
//             return( "SSL - Connection requires a write call" );
//         case -(MBEDTLS_ERR_SSL_TIMEOUT):
//             return( "SSL - The operation timed out" );
//         case -(MBEDTLS_ERR_SSL_CLIENT_RECONNECT):
//             return( "SSL - The client initiated a reconnect from the same port" );
//         case -(MBEDTLS_ERR_SSL_UNEXPECTED_RECORD):
//             return( "SSL - Record header looks valid but is not expected" );
//         case -(MBEDTLS_ERR_SSL_NON_FATAL):
//             return( "SSL - The alert message received indicates a non-fatal error" );
//         case -(MBEDTLS_ERR_SSL_ILLEGAL_PARAMETER):
//             return( "SSL - A field in a message was incorrect or inconsistent with other fields" );
//         case -(MBEDTLS_ERR_SSL_CONTINUE_PROCESSING):
//             return( "SSL - Internal-only message signaling that further message-processing should be done" );
//         case -(MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS):
//             return( "SSL - The asynchronous operation is not completed yet" );
//         case -(MBEDTLS_ERR_SSL_EARLY_MESSAGE):
//             return( "SSL - Internal-only message signaling that a message arrived early" );
//         case -(MBEDTLS_ERR_SSL_UNEXPECTED_CID):
//             return( "SSL - An encrypted DTLS-frame with an unexpected CID was received" );
//         case -(MBEDTLS_ERR_SSL_VERSION_MISMATCH):
//             return( "SSL - An operation failed due to an unexpected version or configuration" );
//         case -(MBEDTLS_ERR_SSL_BAD_CONFIG):
//             return( "SSL - Invalid value in SSL config" );
// #endif /* MBEDTLS_SSL_TLS_C */

// #if defined(MBEDTLS_X509_USE_C) || defined(MBEDTLS_X509_CREATE_C)
//         case -(MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE):
//             return( "X509 - Unavailable feature, e.g. RSA hashing/encryption combination" );
//         case -(MBEDTLS_ERR_X509_UNKNOWN_OID):
//             return( "X509 - Requested OID is unknown" );
//         case -(MBEDTLS_ERR_X509_INVALID_FORMAT):
//             return( "X509 - The CRT/CRL/CSR format is invalid, e.g. different type expected" );
//         case -(MBEDTLS_ERR_X509_INVALID_VERSION):
//             return( "X509 - The CRT/CRL/CSR version element is invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_SERIAL):
//             return( "X509 - The serial tag or value is invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_ALG):
//             return( "X509 - The algorithm tag or value is invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_NAME):
//             return( "X509 - The name tag or value is invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_DATE):
//             return( "X509 - The date tag or value is invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_SIGNATURE):
//             return( "X509 - The signature tag or value invalid" );
//         case -(MBEDTLS_ERR_X509_INVALID_EXTENSIONS):
//             return( "X509 - The extension tag or value is invalid" );
//         case -(MBEDTLS_ERR_X509_UNKNOWN_VERSION):
//             return( "X509 - CRT/CRL/CSR has an unsupported version number" );
//         case -(MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG):
//             return( "X509 - Signature algorithm (oid) is unsupported" );
//         case -(MBEDTLS_ERR_X509_SIG_MISMATCH):
//             return( "X509 - Signature algorithms do not match. (see \\c ::mbedtls_x509_crt sig_oid)" );
//         case -(MBEDTLS_ERR_X509_CERT_VERIFY_FAILED):
//             return( "X509 - Certificate verification failed, e.g. CRL, CA or signature check failed" );
//         case -(MBEDTLS_ERR_X509_CERT_UNKNOWN_FORMAT):
//             return( "X509 - Format not recognized as DER or PEM" );
//         case -(MBEDTLS_ERR_X509_BAD_INPUT_DATA):
//             return( "X509 - Input invalid" );
//         case -(MBEDTLS_ERR_X509_ALLOC_FAILED):
//             return( "X509 - Allocation of memory failed" );
//         case -(MBEDTLS_ERR_X509_FILE_IO_ERROR):
//             return( "X509 - Read/write of file failed" );
//         case -(MBEDTLS_ERR_X509_BUFFER_TOO_SMALL):
//             return( "X509 - Destination buffer is too small" );
//         case -(MBEDTLS_ERR_X509_FATAL_ERROR):
//             return( "X509 - A fatal error occurred, eg the chain is too long or the vrfy callback failed" );
// #endif /* MBEDTLS_X509_USE_C || MBEDTLS_X509_CREATE_C */
//         /* End Auto-Generated Code. */

//         default:
//             break;
//     }

//     return NULL;
// }

// const char *mbedtls_low_level_strerr(int error_code)
// {
//     int low_level_error_code;

//     if (error_code < 0) {
//         error_code = -error_code;
//     }

//     /* Extract the low-level part from the error code. */
//     low_level_error_code = error_code & ~0xFF80;

//     switch (low_level_error_code) {
//     /* Begin Auto-Generated Code. */
//     #if defined(MBEDTLS_AES_C)
//         case -(MBEDTLS_ERR_AES_INVALID_KEY_LENGTH):
//             return( "AES - Invalid key length" );
//         case -(MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH):
//             return( "AES - Invalid data input length" );
//         case -(MBEDTLS_ERR_AES_BAD_INPUT_DATA):
//             return( "AES - Invalid input data" );
// #endif /* MBEDTLS_AES_C */

// #if defined(MBEDTLS_ARIA_C)
//         case -(MBEDTLS_ERR_ARIA_BAD_INPUT_DATA):
//             return( "ARIA - Bad input data" );
//         case -(MBEDTLS_ERR_ARIA_INVALID_INPUT_LENGTH):
//             return( "ARIA - Invalid data input length" );
// #endif /* MBEDTLS_ARIA_C */

// #if defined(MBEDTLS_ASN1_PARSE_C)
//         case -(MBEDTLS_ERR_ASN1_OUT_OF_DATA):
//             return( "ASN1 - Out of data when parsing an ASN1 data structure" );
//         case -(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG):
//             return( "ASN1 - ASN1 tag was of an unexpected value" );
//         case -(MBEDTLS_ERR_ASN1_INVALID_LENGTH):
//             return( "ASN1 - Error when trying to determine the length or invalid length" );
//         case -(MBEDTLS_ERR_ASN1_LENGTH_MISMATCH):
//             return( "ASN1 - Actual length differs from expected length" );
//         case -(MBEDTLS_ERR_ASN1_INVALID_DATA):
//             return( "ASN1 - Data is invalid" );
//         case -(MBEDTLS_ERR_ASN1_ALLOC_FAILED):
//             return( "ASN1 - Memory allocation failed" );
//         case -(MBEDTLS_ERR_ASN1_BUF_TOO_SMALL):
//             return( "ASN1 - Buffer too small when writing ASN.1 data structure" );
// #endif /* MBEDTLS_ASN1_PARSE_C */

// #if defined(MBEDTLS_BASE64_C)
//         case -(MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL):
//             return( "BASE64 - Output buffer too small" );
//         case -(MBEDTLS_ERR_BASE64_INVALID_CHARACTER):
//             return( "BASE64 - Invalid character in input" );
// #endif /* MBEDTLS_BASE64_C */

// #if defined(MBEDTLS_BIGNUM_C)
//         case -(MBEDTLS_ERR_MPI_FILE_IO_ERROR):
//             return( "BIGNUM - An error occurred while reading from or writing to a file" );
//         case -(MBEDTLS_ERR_MPI_BAD_INPUT_DATA):
//             return( "BIGNUM - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_MPI_INVALID_CHARACTER):
//             return( "BIGNUM - There is an invalid character in the digit string" );
//         case -(MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL):
//             return( "BIGNUM - The buffer is too small to write to" );
//         case -(MBEDTLS_ERR_MPI_NEGATIVE_VALUE):
//             return( "BIGNUM - The input arguments are negative or result in illegal output" );
//         case -(MBEDTLS_ERR_MPI_DIVISION_BY_ZERO):
//             return( "BIGNUM - The input argument for division is zero, which is not allowed" );
//         case -(MBEDTLS_ERR_MPI_NOT_ACCEPTABLE):
//             return( "BIGNUM - The input arguments are not acceptable" );
//         case -(MBEDTLS_ERR_MPI_ALLOC_FAILED):
//             return( "BIGNUM - Memory allocation failed" );
// #endif /* MBEDTLS_BIGNUM_C */

// #if defined(MBEDTLS_CAMELLIA_C)
//         case -(MBEDTLS_ERR_CAMELLIA_BAD_INPUT_DATA):
//             return( "CAMELLIA - Bad input data" );
//         case -(MBEDTLS_ERR_CAMELLIA_INVALID_INPUT_LENGTH):
//             return( "CAMELLIA - Invalid data input length" );
// #endif /* MBEDTLS_CAMELLIA_C */

// #if defined(MBEDTLS_CCM_C)
//         case -(MBEDTLS_ERR_CCM_BAD_INPUT):
//             return( "CCM - Bad input parameters to the function" );
//         case -(MBEDTLS_ERR_CCM_AUTH_FAILED):
//             return( "CCM - Authenticated decryption failed" );
// #endif /* MBEDTLS_CCM_C */

// #if defined(MBEDTLS_CHACHA20_C)
//         case -(MBEDTLS_ERR_CHACHA20_BAD_INPUT_DATA):
//             return( "CHACHA20 - Invalid input parameter(s)" );
// #endif /* MBEDTLS_CHACHA20_C */

// #if defined(MBEDTLS_CHACHAPOLY_C)
//         case -(MBEDTLS_ERR_CHACHAPOLY_BAD_STATE):
//             return( "CHACHAPOLY - The requested operation is not permitted in the current state" );
//         case -(MBEDTLS_ERR_CHACHAPOLY_AUTH_FAILED):
//             return( "CHACHAPOLY - Authenticated decryption failed: data was not authentic" );
// #endif /* MBEDTLS_CHACHAPOLY_C */

// #if defined(MBEDTLS_CTR_DRBG_C)
//         case -(MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED):
//             return( "CTR_DRBG - The entropy source failed" );
//         case -(MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG):
//             return( "CTR_DRBG - The requested random buffer length is too big" );
//         case -(MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG):
//             return( "CTR_DRBG - The input (entropy + additional data) is too large" );
//         case -(MBEDTLS_ERR_CTR_DRBG_FILE_IO_ERROR):
//             return( "CTR_DRBG - Read or write error in file" );
// #endif /* MBEDTLS_CTR_DRBG_C */

// #if defined(MBEDTLS_DES_C)
//         case -(MBEDTLS_ERR_DES_INVALID_INPUT_LENGTH):
//             return( "DES - The data input has an invalid length" );
// #endif /* MBEDTLS_DES_C */

// #if defined(MBEDTLS_ENTROPY_C)
//         case -(MBEDTLS_ERR_ENTROPY_SOURCE_FAILED):
//             return( "ENTROPY - Critical entropy source failure" );
//         case -(MBEDTLS_ERR_ENTROPY_MAX_SOURCES):
//             return( "ENTROPY - No more sources can be added" );
//         case -(MBEDTLS_ERR_ENTROPY_NO_SOURCES_DEFINED):
//             return( "ENTROPY - No sources have been added to poll" );
//         case -(MBEDTLS_ERR_ENTROPY_NO_STRONG_SOURCE):
//             return( "ENTROPY - No strong sources have been added to poll" );
//         case -(MBEDTLS_ERR_ENTROPY_FILE_IO_ERROR):
//             return( "ENTROPY - Read/write error in file" );
// #endif /* MBEDTLS_ENTROPY_C */

// #if defined(MBEDTLS_ERROR_C)
//         case -(MBEDTLS_ERR_ERROR_GENERIC_ERROR):
//             return( "ERROR - Generic error" );
//         case -(MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED):
//             return( "ERROR - This is a bug in the library" );
// #endif /* MBEDTLS_ERROR_C */

// #if defined(MBEDTLS_PLATFORM_C)
//         case -(MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED):
//             return( "PLATFORM - Hardware accelerator failed" );
//         case -(MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED):
//             return( "PLATFORM - The requested feature is not supported by the platform" );
// #endif /* MBEDTLS_PLATFORM_C */

// #if defined(MBEDTLS_GCM_C)
//         case -(MBEDTLS_ERR_GCM_AUTH_FAILED):
//             return( "GCM - Authenticated decryption failed" );
//         case -(MBEDTLS_ERR_GCM_BAD_INPUT):
//             return( "GCM - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_GCM_BUFFER_TOO_SMALL):
//             return( "GCM - An output buffer is too small" );
// #endif /* MBEDTLS_GCM_C */

// #if defined(MBEDTLS_HKDF_C)
//         case -(MBEDTLS_ERR_HKDF_BAD_INPUT_DATA):
//             return( "HKDF - Bad input parameters to function" );
// #endif /* MBEDTLS_HKDF_C */

// #if defined(MBEDTLS_HMAC_DRBG_C)
//         case -(MBEDTLS_ERR_HMAC_DRBG_REQUEST_TOO_BIG):
//             return( "HMAC_DRBG - Too many random requested in single call" );
//         case -(MBEDTLS_ERR_HMAC_DRBG_INPUT_TOO_BIG):
//             return( "HMAC_DRBG - Input too large (Entropy + additional)" );
//         case -(MBEDTLS_ERR_HMAC_DRBG_FILE_IO_ERROR):
//             return( "HMAC_DRBG - Read/write error in file" );
//         case -(MBEDTLS_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED):
//             return( "HMAC_DRBG - The entropy source failed" );
// #endif /* MBEDTLS_HMAC_DRBG_C */

// #if defined(MBEDTLS_LMS_C)
//         case -(MBEDTLS_ERR_LMS_BAD_INPUT_DATA):
//             return( "LMS - Bad data has been input to an LMS function" );
//         case -(MBEDTLS_ERR_LMS_OUT_OF_PRIVATE_KEYS):
//             return( "LMS - Specified LMS key has utilised all of its private keys" );
//         case -(MBEDTLS_ERR_LMS_VERIFY_FAILED):
//             return( "LMS - LMS signature verification failed" );
//         case -(MBEDTLS_ERR_LMS_ALLOC_FAILED):
//             return( "LMS - LMS failed to allocate space for a private key" );
//         case -(MBEDTLS_ERR_LMS_BUFFER_TOO_SMALL):
//             return( "LMS - Input/output buffer is too small to contain requited data" );
// #endif /* MBEDTLS_LMS_C */

// #if defined(MBEDTLS_NET_C)
//         case -(MBEDTLS_ERR_NET_SOCKET_FAILED):
//             return( "NET - Failed to open a socket" );
//         case -(MBEDTLS_ERR_NET_CONNECT_FAILED):
//             return( "NET - The connection to the given server / port failed" );
//         case -(MBEDTLS_ERR_NET_BIND_FAILED):
//             return( "NET - Binding of the socket failed" );
//         case -(MBEDTLS_ERR_NET_LISTEN_FAILED):
//             return( "NET - Could not listen on the socket" );
//         case -(MBEDTLS_ERR_NET_ACCEPT_FAILED):
//             return( "NET - Could not accept the incoming connection" );
//         case -(MBEDTLS_ERR_NET_RECV_FAILED):
//             return( "NET - Reading information from the socket failed" );
//         case -(MBEDTLS_ERR_NET_SEND_FAILED):
//             return( "NET - Sending information through the socket failed" );
//         case -(MBEDTLS_ERR_NET_CONN_RESET):
//             return( "NET - Connection was reset by peer" );
//         case -(MBEDTLS_ERR_NET_UNKNOWN_HOST):
//             return( "NET - Failed to get an IP address for the given hostname" );
//         case -(MBEDTLS_ERR_NET_BUFFER_TOO_SMALL):
//             return( "NET - Buffer is too small to hold the data" );
//         case -(MBEDTLS_ERR_NET_INVALID_CONTEXT):
//             return( "NET - The context is invalid, eg because it was free()ed" );
//         case -(MBEDTLS_ERR_NET_POLL_FAILED):
//             return( "NET - Polling the net context failed" );
//         case -(MBEDTLS_ERR_NET_BAD_INPUT_DATA):
//             return( "NET - Input invalid" );
// #endif /* MBEDTLS_NET_C */

// #if defined(MBEDTLS_OID_C)
//         case -(MBEDTLS_ERR_OID_NOT_FOUND):
//             return( "OID - OID is not found" );
//         case -(MBEDTLS_ERR_OID_BUF_TOO_SMALL):
//             return( "OID - output buffer is too small" );
// #endif /* MBEDTLS_OID_C */

// #if defined(MBEDTLS_POLY1305_C)
//         case -(MBEDTLS_ERR_POLY1305_BAD_INPUT_DATA):
//             return( "POLY1305 - Invalid input parameter(s)" );
// #endif /* MBEDTLS_POLY1305_C */

// #if defined(MBEDTLS_SHA1_C)
//         case -(MBEDTLS_ERR_SHA1_BAD_INPUT_DATA):
//             return( "SHA1 - SHA-1 input data was malformed" );
// #endif /* MBEDTLS_SHA1_C */

// #if defined(MBEDTLS_SHA256_C)
//         case -(MBEDTLS_ERR_SHA256_BAD_INPUT_DATA):
//             return( "SHA256 - SHA-256 input data was malformed" );
// #endif /* MBEDTLS_SHA256_C */

// #if defined(MBEDTLS_SHA3_C)
//         case -(MBEDTLS_ERR_SHA3_BAD_INPUT_DATA):
//             return( "SHA3 - SHA-3 input data was malformed" );
// #endif /* MBEDTLS_SHA3_C */

// #if defined(MBEDTLS_SHA512_C)
//         case -(MBEDTLS_ERR_SHA512_BAD_INPUT_DATA):
//             return( "SHA512 - SHA-512 input data was malformed" );
// #endif /* MBEDTLS_SHA512_C */

// #if defined(MBEDTLS_THREADING_C)
//         case -(MBEDTLS_ERR_THREADING_BAD_INPUT_DATA):
//             return( "THREADING - Bad input parameters to function" );
//         case -(MBEDTLS_ERR_THREADING_MUTEX_ERROR):
//             return( "THREADING - Locking / unlocking / free failed with error code" );
// #endif /* MBEDTLS_THREADING_C */
//         /* End Auto-Generated Code. */

//         default:
//             break;
//     }

//     return NULL;
// }


#include <stdio.h>

//int mbedtls_platform_entropy_poll(void *data,
//                                  unsigned char *output, size_t len, size_t *olen)
//{
//    FILE *file;
//    size_t read_len;
//    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
//    ((void) data);
//
//#if defined(HAVE_GETRANDOM)
//    ret = getrandom_wrapper(output, len, 0);
//    if (ret >= 0) {
//        *olen = (size_t) ret;
//        return 0;
//    } else if (errno != ENOSYS) {
//        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
//    }
//    /* Fall through if the system call isn't known. */
//#else
//    ((void) ret);
//#endif /* HAVE_GETRANDOM */
//
//#if defined(HAVE_SYSCTL_ARND)
//    ((void) file);
//    ((void) read_len);
//    if (sysctl_arnd_wrapper(output, len) == -1) {
//        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
//    }
//    *olen = len;
//    return 0;
//#else
//
//    *olen = 0;
//
//    file = fopen("/dev/urandom", "rb");
//    if (file == NULL) {
//        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
//    }
//
//    /* Ensure no stdio buffering of secrets, as such buffers cannot be wiped. */
//    //mbedtls_setbuf(file, NULL);
//
//    read_len = fread(output, 1, len, file);
//    if (read_len != len) {
//        fclose(file);
//        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
//    }
//
//    fclose(file);
//    *olen = len;
//
//    return 0;
//#endif /* HAVE_SYSCTL_ARND */
//}



int mbedtls_null_entropy_poll( void *data,
                    unsigned char *output, size_t len, size_t *olen )
{
    ((void) data);
    ((void) output);
    *olen = 0;
    if( len < sizeof(unsigned char) )
        return( 0 );
    *olen = sizeof(unsigned char);
    return( 0 );
}







#define MBEDTLS_ENTROPY_C






/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a high-level code.
 */
static const char * pcNoHighLevelMbedTlsCodeStr = "<No-High-Level-Code>";

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a low-level code.
 */
static const char * pcNoLowLevelMbedTlsCodeStr = "<No-Low-Level-Code>";

/**
 * @brief Utility for converting the high-level code in an mbedTLS error to string,
 * if the code-contains a high-level code; otherwise, using a default string.
 */
#define mbedtlsHighLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_high_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_high_level_strerr( mbedTlsCode ) : pcNoHighLevelMbedTlsCodeStr

/**
 * @brief Utility for converting the level-level code in an mbedTLS error to string,
 * if the code-contains a level-level code; otherwise, using a default string.
 */
#define mbedtlsLowLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_low_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_low_level_strerr( mbedTlsCode ) : pcNoLowLevelMbedTlsCodeStr

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the mbed TLS structures in a network connection.
 *
 * @param[in] pxSslContext The SSL context to initialize.
 */
static void sslContextInit( MbedSSLContext_t * pxSslContext );

/**
 * @brief Free the mbed TLS structures in a network connection.
 *
 * @param[in] pxSslContext The SSL context to free.
 */
static void sslContextFree( MbedSSLContext_t * pxSslContext );

/**
 * @brief Add X509 certificate to the trusted list of root certificates.
 *
 * OpenSSL does not provide a single function for reading and loading certificates
 * from files into stores, so the file API must be called. Start with the
 * root certificate.
 *
 * @param[out] pxSslContext SSL context to which the trusted server root CA is to be added.
 * @param[in] pucRootCa PEM-encoded string of the trusted server root CA.
 * @param[in] xRootCaSize Size of the trusted server root CA.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setRootCa( MbedSSLContext_t * pxSslContext,
                          const uint8_t * pucRootCa,
                          size_t xRootCaSize );

/**
 * @brief Set X509 certificate as client certificate for the server to authenticate.
 *
 * @param[out] pxSslContext SSL context to which the client certificate is to be set.
 * @param[in] pucClientCert PEM-encoded string of the client certificate.
 * @param[in] xClientCertSize Size of the client certificate.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setClientCertificate( MbedSSLContext_t * pxSslContext,
                                     const uint8_t * pucClientCert,
                                     size_t xClientCertSize );

/**
 * @brief Set private key for the client's certificate.
 *
 * @param[out] pxSslContext SSL context to which the private key is to be set.
 * @param[in] pucPrivateKey PEM-encoded string of the client private key.
 * @param[in] xPrivateKeySize Size of the client private key.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setPrivateKey( MbedSSLContext_t * pxSslContext,
                              const uint8_t * pucPrivateKey,
                              size_t xPrivateKeySize );

/**
 * @brief Passes TLS credentials to the OpenSSL library.
 *
 * Provides the root CA certificate, client certificate, and private key to the
 * OpenSSL library. If the client certificate or private key is not NULL, mutual
 * authentication is used when performing the TLS handshake.
 *
 * @param[out] pxSslContext SSL context to which the credentials are to be imported.
 * @param[in] pxNetworkCredentials TLS credentials to be imported.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setCredentials( MbedSSLContext_t * pxSslContext,
                               const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Set optional configurations for the TLS connection.
 *
 * This function is used to set SNI and ALPN protocols.
 *
 * @param[in] pxSslContext SSL context to which the optional configurations are to be set.
 * @param[in] pcHostName Remote host name, used for server name indication.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 */
static void setOptionalConfigurations( MbedSSLContext_t * pxSslContext,
                                       const char * pcHostName,
                                       const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Setup TLS by initializing contexts and setting configurations.
 *
 * @param[in] pxNetworkContext Network context.
 * @param[in] pcHostName Remote host name, used for server name indication.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 *
 * @return #eTLSTransportSuccess, #eTLSTransportInsufficientMemory, #eTLSTransportInvalidCredentials,
 * or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t tlsSetup( NetworkContext_t * pxNetworkContext,
                                      const char * pcHostName,
                                      const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Perform the TLS handshake on a TCP connection.
 *
 * @param[in] pxNetworkContext Network context.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 *
 * @return #eTLSTransportSuccess, #eTLSTransportHandshakeFailed, or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pxNetworkContext,
                                          const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Initialize mbedTLS.
 *
 * @param[out] entropyContext mbed TLS entropy context for generation of random numbers.
 * @param[out] ctrDrgbContext mbed TLS CTR DRBG context for generation of random numbers.
 *
 * @return #eTLSTransportSuccess, or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pxEntropyContext,
                                         mbedtls_ctr_drbg_context * pxCtrDrgbContext );

/*-----------------------------------------------------------*/

#include <mbedtls/debug.h>

#define WIFI_PLATFORM_MBEDTLS_DEBUG_LOG_LEVEL    10

static void mbedtlsDebugPrint( void * ctx,
                               int level,
                               const char * pFile,
                               int line,
                               const char * pStr )
{
    extern int Report(const char *pcFormat,...);
    Report("\n\rwifi_platform_mbedtls: |%d| %s \n\r", level, pStr);
}

// Add this verification callback
int my_verify_callback(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    char buf[1536];
    Report("\n\rVerifying certificate at depth %d:\n\r", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    Report("%s", buf);
    
    if(*flags != 0) {
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
        Report("%s\n", buf);
    }
    else
    {
        Report("\n\rParent is NULL\n\r");
    }
    
    return 0; // Return 0 to continue despite errors
}

static void sslContextInit( MbedSSLContext_t * pxSslContext )
{
    configASSERT( pxSslContext != NULL );

    mbedtls_ssl_config_init( &( pxSslContext->config ) );
#if 0
    //mbedtls debug
    mbedtls_ssl_conf_dbg(&( pxSslContext->config ), mbedtlsDebugPrint, NULL);
	mbedtls_debug_set_threshold(WIFI_PLATFORM_MBEDTLS_DEBUG_LOG_LEVEL);
#endif
#if 0    
    // Register the callback
    mbedtls_ssl_conf_verify(&( pxSslContext->config ), my_verify_callback, NULL);
#endif
    mbedtls_x509_crt_init( &( pxSslContext->rootCa ) );
    mbedtls_pk_init( &( pxSslContext->privKey ) );
    mbedtls_x509_crt_init( &( pxSslContext->clientCert ) );
    mbedtls_ssl_init( &( pxSslContext->context ) );
}
/*-----------------------------------------------------------*/

static void sslContextFree( MbedSSLContext_t * pxSslContext )
{
    configASSERT( pxSslContext != NULL );

    mbedtls_ssl_free( &( pxSslContext->context ) );
    mbedtls_x509_crt_free( &( pxSslContext->rootCa ) );
    mbedtls_x509_crt_free( &( pxSslContext->clientCert ) );
    mbedtls_pk_free( &( pxSslContext->privKey ) );
#if 0   
    mbedtls_entropy_free( &( pxSslContext->entropyContext ) );
    mbedtls_ctr_drbg_free( &( pxSslContext->ctrDrgbContext ) );
#endif
    mbedtls_ssl_config_free( &( pxSslContext->config ) );
}
/*-----------------------------------------------------------*/

static int32_t setRootCa( MbedSSLContext_t * pxSslContext,
                          const uint8_t * pucRootCa,
                          size_t xRootCaSize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucRootCa != NULL );

    /* Parse the server root CA certificate into the SSL context. */
    lMbedtlsError = mbedtls_x509_crt_parse( &( pxSslContext->rootCa ),
                                            pucRootCa,
                                            xRootCaSize );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse server root CA certificate: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        mbedtls_ssl_conf_ca_chain( &( pxSslContext->config ),
                                   &( pxSslContext->rootCa ),
                                   NULL );
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setClientCertificate( MbedSSLContext_t * pxSslContext,
                                     const uint8_t * pucClientCert,
                                     size_t xClientCertSize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucClientCert != NULL );

    /* Setup the client certificate. */
    lMbedtlsError = mbedtls_x509_crt_parse( &( pxSslContext->clientCert ),
                                            pucClientCert,
                                            xClientCertSize );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client certificate: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }

    return lMbedtlsError;
}

#if 1
int crypto_psa_get_random(void *params, unsigned char *randBuff, size_t randLen);
#endif
/*-----------------------------------------------------------*/

static int32_t setPrivateKey( MbedSSLContext_t * pxSslContext,
                              const uint8_t * pucPrivateKey,
                              size_t xPrivateKeySize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucPrivateKey != NULL );

    /* Setup the client private key. */
    lMbedtlsError = mbedtls_pk_parse_key( &( pxSslContext->privKey ),
                                          pucPrivateKey,
                                          xPrivateKeySize,
                                          NULL,
                                          0, crypto_psa_get_random/*mbedtls_ctr_drbg_random*/, NULL/* pxSslContext->ctrDrgbContext */ );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client key: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setCredentials( MbedSSLContext_t * pxSslContext,
                               const NetworkCredentials_t * pxNetworkCredentials )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    /* Set up the certificate security profile, starting from the default value. */
    pxSslContext->certProfile = mbedtls_x509_crt_profile_default;

    /* Set SSL authmode and the RNG context. */
    /* Not allowed with MBEDTLS 1.3. MUST verify. */
    // mbedtls_ssl_conf_authmode( &( pxSslContext->config ),
    //                           MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_rng( &( pxSslContext->config ),
                          crypto_psa_get_random/*mbedtls_ctr_drbg_random*/,
                          &( pxSslContext->ctrDrgbContext ) );
    mbedtls_ssl_conf_cert_profile( &( pxSslContext->config ),
                                   &( pxSslContext->certProfile ) );


    lMbedtlsError = setRootCa( pxSslContext,
                               pxNetworkCredentials->pucRootCa,
                               pxNetworkCredentials->xRootCaSize );

    if( ( pxNetworkCredentials->pucClientCert != NULL ) &&
        ( pxNetworkCredentials->pucPrivateKey != NULL ) )
    {
        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = setClientCertificate( pxSslContext,
                                                  pxNetworkCredentials->pucClientCert,
                                                  pxNetworkCredentials->xClientCertSize );
        }

        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = setPrivateKey( pxSslContext,
                                           pxNetworkCredentials->pucPrivateKey,
                                           pxNetworkCredentials->xPrivateKeySize );
        }

        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = mbedtls_ssl_conf_own_cert( &( pxSslContext->config ),
                                                       &( pxSslContext->clientCert ),
                                                       &( pxSslContext->privKey ) );
        }
    }
    UART_PRINT("CREDENTIALS SET \r\n");
    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static void setOptionalConfigurations( MbedSSLContext_t * pxSslContext,
                                       const char * pcHostName,
                                       const NetworkCredentials_t * pxNetworkCredentials )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pcHostName != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    if( pxNetworkCredentials->ppcAlpnProtos != NULL )
    {
        /* Include an application protocol list in the TLS ClientHello
         * message. */
        lMbedtlsError = mbedtls_ssl_conf_alpn_protocols( &( pxSslContext->config ),
                                                         pxNetworkCredentials->ppcAlpnProtos );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to configure ALPN protocol in mbed TLS: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    }

    /* Enable SNI if requested. */
    if( pxNetworkCredentials->xDisableSni == pdFALSE )
    {
        lMbedtlsError = mbedtls_ssl_set_hostname( &( pxSslContext->context ),
                                                  pcHostName );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to set server name: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
        else {
            UART_PRINT("HOSTNAME SET \r\n");
        }
    }

    /* Set Maximum Fragment Length if enabled. */
    #ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

        /* Enable the max fragment extension. 4096 bytes is currently the largest fragment size permitted.
         * See RFC 8449 https://tools.ietf.org/html/rfc8449 for more information.
         *
         * Smaller values can be found in "mbedtls/include/ssl.h".
         */
        lMbedtlsError = mbedtls_ssl_conf_max_frag_len( &( pxSslContext->config ), MBEDTLS_SSL_MAX_FRAG_LEN_4096 );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to maximum fragment length extension: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    #endif /* ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsSetup( NetworkContext_t * pxNetworkContext,
                                      const char * pcHostName,
                                      const NetworkCredentials_t * pxNetworkCredentials )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext = NULL;

    configASSERT( pxNetworkContext != NULL );
    configASSERT( pxNetworkContext->pParams != NULL );
    configASSERT( pcHostName != NULL );
    configASSERT( pxNetworkCredentials != NULL );
    configASSERT( pxNetworkCredentials->pucRootCa != NULL );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;
    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Initialize the mbed TLS context structures. */
    sslContextInit( pxSSLContext );

    lMbedtlsError = mbedtls_ssl_config_defaults( &( pxSSLContext->config ),
                                                 MBEDTLS_SSL_IS_CLIENT,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to set default SSL configuration: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Per mbed TLS docs, mbedtls_ssl_config_defaults only fails on memory allocation. */
        xRetVal = eTLSTransportInsufficientMemory;
    }

    /* Enable TLS1.3 */
    mbedtls_ssl_conf_min_tls_version(&( pxSSLContext->config ), MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&( pxSSLContext->config ), MBEDTLS_SSL_VERSION_TLS1_2);

    #define READ_TIMEOUT_MS 10000
    #define OPT_TICKETS MBEDTLS_SSL_SESSION_TICKETS_ENABLED

    mbedtls_ssl_conf_read_timeout( &(pxSSLContext->config), READ_TIMEOUT_MS);
    mbedtls_ssl_conf_session_tickets( &(pxSSLContext->config), OPT_TICKETS );

    mbedtls_ssl_conf_renegotiation( &(pxSSLContext->config), MBEDTLS_SSL_RENEGOTIATION_ENABLED);

    if( xRetVal == eTLSTransportSuccess )
    {
        lMbedtlsError = setCredentials( pxSSLContext,
                                        pxNetworkCredentials );

        if( lMbedtlsError != 0 )
        {
            xRetVal = eTLSTransportInvalidCredentials;
        }
        else
        {
            /* Optionally set SNI and ALPN protocols. */
            setOptionalConfigurations( pxSSLContext,
                                       pcHostName,
                                       pxNetworkCredentials );
        }

        // vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pxNetworkContext,
                                          const NetworkCredentials_t * pxNetworkCredentials )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext = NULL;

    configASSERT( pxNetworkContext != NULL );
    configASSERT( pxNetworkContext->pParams != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;
    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Initialize the mbed TLS secured connection context. */
    lMbedtlsError = mbedtls_ssl_setup( &( pxSSLContext->context ),
                                       &( pxSSLContext->config ) );
    
    // vTaskDelay(pdMS_TO_TICKS(1000));

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to set up mbed TLS SSL context: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        xRetVal = eTLSTransportInternalError;
    }
    else
    {
        /* Set the underlying IO for the TLS connection. */

        /* MISRA Rule 11.2 flags the following line for casting the second
         * parameter to void *. This rule is suppressed because
         * #mbedtls_ssl_set_bio requires the second parameter as void *.
         */
        /* coverity[misra_c_2012_rule_11_2_violation] */
        mbedtls_ssl_set_bio( &( pxSSLContext->context ),
                             ( void * ) pxTlsTransportParams->xTCPSocket,
                             mbedtls_platform_send,
                             mbedtls_platform_recv,
                             NULL );
    }

    // vTaskDelay(pdMS_TO_TICKS(1000));

    if( xRetVal == eTLSTransportSuccess )
    {
        /* Perform the TLS handshake. */
        while ((lMbedtlsError = mbedtls_ssl_handshake(&(pxSSLContext->context))) != 0) {
            if (lMbedtlsError != MBEDTLS_ERR_SSL_WANT_READ && lMbedtlsError != MBEDTLS_ERR_SSL_WANT_WRITE) {
                UART_PRINT("Failed to handshake mbedtls with SSL. -0x%x\n\r",
                           (unsigned int) -lMbedtlsError);
                uint32_t flags = mbedtls_ssl_get_verify_result(&(pxSSLContext->context));
                char info[512];
                mbedtls_x509_crt_verify_info(info, sizeof(info), "  ! ", flags);
                UART_PRINT("VERIFY FLAGS: %s\r\n", info);
                break;
            }
            // Yield to other FreeRTOS tasks during handshake
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        // do
        // {
        //    lMbedtlsError = mbedtls_ssl_handshake( &( pxSSLContext->context ) );
        // } while( ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
        //         ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to perform TLS handshake: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

            if( lMbedtlsError == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED )
            {
                xRetVal = eTLSTransportCAVerifyFailed;
            }
            else
            {
                xRetVal = eTLSTransportHandshakeFailed;
            }
        }
        else
        {
            LogInfo( ( "(Network connection %p) TLS handshake successful.",
                       pxNetworkContext ) );
        }
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pxEntropyContext,
                                         mbedtls_ctr_drbg_context * pxCtrDrgbContext )
{
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;
#if 1
    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt(threading_mutex_init_pthread,
                            threading_mutex_free_pthread,
                            threading_mutex_lock_pthread,
                            threading_mutex_unlock_pthread);
#endif
#if 0
    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );
#endif                               
#if 0
    mbedtls_platform_set_calloc_free(mbedtls_platform_calloc, mbedtls_platform_free);

    /* Initialize contexts for random number generation. */
    mbedtls_entropy_init( pxEntropyContext );
    mbedtls_ctr_drbg_init( pxCtrDrgbContext );

    /* Add a strong entropy source. At least one is required. */
    lMbedtlsError = mbedtls_entropy_add_source( pxEntropyContext,
                                                mbedtls_null_entropy_poll,
                                                NULL,
                                                32,
                                                MBEDTLS_ENTROPY_SOURCE_STRONG );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to add entropy source: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        xRetVal = eTLSTransportInternalError;
    }
#endif
    /* Need to initialize PSA as part of initializing mebedTLS stack. */
    psa_status_t status = psa_crypto_init();

    if (status != PSA_SUCCESS)
    {
        LogError( ( "Failed to init PSA Crypto: status[%d].", status));
        xRetVal = eTLSTransportInternalError;
    }
#if 0
    if( xRetVal == eTLSTransportSuccess )
    {
        /* Seed the random number generator. */
        lMbedtlsError = mbedtls_ctr_drbg_seed( pxCtrDrgbContext,
                                               mbedtls_entropy_func,
                                               pxEntropyContext,
                                               NULL,
                                               0 );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to seed PRNG: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
            xRetVal = eTLSTransportInternalError;
        }
    }
#endif
    if( xRetVal == eTLSTransportSuccess )
    {
        LogDebug( ( "Successfully initialized mbedTLS." ) );
    }
    return xRetVal;
}
/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                         const char * pcHostName,
                                         uint16_t usPort,
                                         const NetworkCredentials_t * pxNetworkCredentials,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    BaseType_t xSocketStatus = 0;
    MbedSSLContext_t * pxSSLContext;
    TickType_t xRecvTimeout = pdMS_TO_TICKS( ulReceiveTimeoutMs );
    TickType_t xSendTimeout = pdMS_TO_TICKS( ulSendTimeoutMs );

    if( ( pxNetworkContext == NULL ) ||
        ( pxNetworkContext->pParams == NULL ) ||
        ( pcHostName == NULL ) ||
        ( pxNetworkCredentials == NULL ) )
    {
        LogError( ( "Invalid input parameter(s): Arguments cannot be NULL. pxNetworkContext=%p, "
                    "pcHostName=%p, pxNetworkCredentials=%p.",
                    pxNetworkCredentials,
                    pcHostName,
                    pxNetworkCredentials ) );
        xRetVal = eTLSTransportInvalidParameter;
    }
    else if( ( pxNetworkCredentials->pucRootCa == NULL ) )
    {
        LogError( ( "pucRootCa cannot be NULL." ) );
        xRetVal = eTLSTransportInvalidParameter;
    }
    else if( ( pxSSLContext = pvPortMalloc( sizeof( MbedSSLContext_t ) ) ) == NULL )
    {
        LogError( ( "Failed to allocate mbed ssl context memmory ." ) );
        xRetVal = eTLSTransportInsufficientMemory;
    }
    else
    {
        pxTlsTransportParams = pxNetworkContext->pParams;
        pxTlsTransportParams->xSSLContext = ( SSLContextHandle ) pxSSLContext;

        if( ( pxTlsTransportParams->xTCPSocket = Sockets_Open() ) == SOCKETS_INVALID_SOCKET )
        {
            LogError( ( "Failed to open socket." ) );
            xRetVal = eTLSTransportConnectFailure;
        }
        else if( ( xSocketStatus = Sockets_SetSockOpt( pxTlsTransportParams->xTCPSocket,
                                                       SOCKETS_SO_RCVTIMEO,
                                                       &xRecvTimeout,
                                                       sizeof( xRecvTimeout ) ) != 0 ) )
        {
            LogError( ( "Failed to set receive timeout on socket %d.", xSocketStatus ) );
            xRetVal = eTLSTransportInternalError;
        }
        else if( ( xSocketStatus = Sockets_SetSockOpt( pxTlsTransportParams->xTCPSocket,
                                                       SOCKETS_SO_SNDTIMEO,
                                                       &xSendTimeout,
                                                       sizeof( xSendTimeout ) ) != 0 ) )
        {
            LogError( ( "Failed to set send timeout on socket %d.", xSocketStatus ) );
            xRetVal = eTLSTransportInternalError;
        }
        else if( ( xSocketStatus = Sockets_Connect( pxTlsTransportParams->xTCPSocket,
                                                    pcHostName,
                                                    usPort ) ) != 0 )
        {
            LogError( ( "Failed to connect to %s with error %d.",
                        pcHostName,
                        xSocketStatus ) );
            xRetVal = eTLSTransportConnectFailure;
        }
        else if( ( xRetVal = initMbedtls( &( pxSSLContext->entropyContext ),
                                          &( pxSSLContext->ctrDrgbContext ) ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to initialize Mbedtls %d.", xRetVal ) );
        }
        else if( ( xRetVal = tlsSetup( pxNetworkContext, pcHostName,
                                       pxNetworkCredentials ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to setup Mbedtls %d.", xRetVal ) );
        }
        else if( ( xRetVal = tlsHandshake( pxNetworkContext, pxNetworkCredentials ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to do TLS handshake %d.", xRetVal ) );
        }
        else
        {
            LogInfo( ( "(Network connection %p) Connection to %s established.",
                       pxNetworkContext,
                       pcHostName ) );
        }

        /* Clean up on failure. */
        if( xRetVal != eTLSTransportSuccess )
        {
            if( ( pxNetworkContext != NULL ) && ( pxNetworkContext->pParams != NULL ) )
            {
                sslContextFree( pxSSLContext );
                vPortFree( pxSSLContext );
                pxTlsTransportParams->xSSLContext = NULL;

                if( pxTlsTransportParams->xTCPSocket != SOCKETS_INVALID_SOCKET )
                {
                    ( void ) Sockets_Disconnect( pxTlsTransportParams->xTCPSocket );
                    ( void ) Sockets_Close( pxTlsTransportParams->xTCPSocket );
                }
            }
        }
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

void TLS_Socket_Disconnect( NetworkContext_t * pxNetworkContext )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;

    if( ( pxNetworkContext == NULL ) || ( pxNetworkContext->pParams != NULL ) )
    {
        /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
        LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                   "received %s as the TLS status can be ignored for close-notify.",
                   ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                   pxNetworkContext ) );
    }

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    if( pxTlsTransportParams->xSSLContext == NULL )
    {
        {
            /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
            LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                       "received %s as the TLS status can be ignored for close-notify.",
                       ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                       pxNetworkContext ) );
        }
    }

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Attempting to terminate TLS connection. */
    lMbedtlsError = mbedtls_ssl_close_notify( &( pxSSLContext->context ) );

    /* Ignore the WANT_READ and WANT_WRITE return values. */
    if( ( lMbedtlsError != MBEDTLS_ERR_SSL_WANT_READ ) &&
        ( lMbedtlsError != MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        if( lMbedtlsError == 0 )
        {
            LogInfo( ( "(Network connection %p) TLS close-notify sent.",
                       pxNetworkContext ) );
        }
        else
        {
            LogError( ( "(Network connection %p) Failed to send TLS close-notify: mbedTLSError[%d]= %s : %s.",
                        pxNetworkContext, lMbedtlsError,
                        mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    }
    else
    {
        /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
        LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                   "received %s as the TLS status can be ignored for close-notify.",
                   ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                   pxNetworkContext ) );
    }

    /* Call socket shutdown function to close connection. */
    Sockets_Disconnect( pxTlsTransportParams->xTCPSocket );
    Sockets_Close( pxTlsTransportParams->xTCPSocket );

    /* Free mbed TLS contexts. */
    sslContextFree( pxSSLContext );
    vPortFree( pxSSLContext );

    /* Clear the mutex functions for mbed TLS thread safety. */
//    mbedtls_threading_free_alt();
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Recv( NetworkContext_t * pxNetworkContext,
                         void * pvBuffer,
                         size_t xBytesToRecv )
{
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;
    TlsTransportParams_t * pxTlsTransportParams = NULL;

    configASSERT( ( pxNetworkContext != NULL ) &&
                  ( pxNetworkContext->pParams != NULL ) );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;
    lMbedtlsError = ( int32_t ) mbedtls_ssl_read( &( pxSSLContext->context ),
                                                  pvBuffer,
                                                  xBytesToRecv );

    if( ( lMbedtlsError == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to read data. However, a read can be retried on this error. "
                    "mbedTLSError[%d]= %s : %s.", lMbedtlsError,
                    mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry read
         * on these errors. */
        lMbedtlsError = 0;
    }
    else if( lMbedtlsError < 0 )
    {
        LogError( ( "Failed to read data: mbedTLSError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Send( NetworkContext_t * pxNetworkContext,
                         const void * pvBuffer,
                         size_t xBytesToSend )
{
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;
    TlsTransportParams_t * pxTlsTransportParams = NULL;

    configASSERT( ( pxNetworkContext != NULL ) &&
                  ( pxNetworkContext->pParams != NULL ) );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;
    lMbedtlsError = ( int32_t ) mbedtls_ssl_write( &( pxSSLContext->context ),
                                                   pvBuffer,
                                                   xBytesToSend );

    if( ( lMbedtlsError == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to send data. However, send can be retried on this error. "
                    "mbedTLSError[%d]= %s : %s.", lMbedtlsError,
                    mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry send
         * on these errors. */
        lMbedtlsError = 0;
    }
    else if( lMbedtlsError < 0 )
    {
        LogError( ( "Failed to send data:  mbedTLSError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/
