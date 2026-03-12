/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* This file configures mbed TLS for FreeRTOS. */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H


/* FreeRTOS include. */
#include "FreeRTOS.h"

/* Set the memory allocation functions on FreeRTOS. */
void * mbedtls_platform_calloc( size_t nmemb,
                                size_t size );
void mbedtls_platform_free( void * ptr );
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO    mbedtls_platform_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO      mbedtls_platform_free

/* The network send and receive functions on FreeRTOS. */
int mbedtls_platform_send( void * ctx,
                           const unsigned char * buf,
                           size_t len );
int mbedtls_platform_recv( void * ctx,
                           unsigned char * buf,
                           size_t len );

/* The entropy poll function. */
int mbedtls_platform_entropy_poll( void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t * olen );

// #include "mbedtls/check_config.h"

#endif /* ifndef MBEDTLS_CONFIG_H */
