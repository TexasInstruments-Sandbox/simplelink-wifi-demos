/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file mbedtls_freertos_port.h
 * @brief mbedTLS FreeRTOS port declarations
 */

#ifndef MBEDTLS_FREERTOS_PORT_H
#define MBEDTLS_FREERTOS_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbedtls/threading.h"

/**
 * @brief Initialize a mutex.
 *
 * @param[in,out] pMutex mbedtls mutex handle.
 */
void mbedtls_platform_mutex_init( mbedtls_threading_mutex_t * pMutex );

/**
 * @brief Free a mutex.
 *
 * @param[in] pMutex mbedtls mutex handle.
 */
void mbedtls_platform_mutex_free( mbedtls_threading_mutex_t * pMutex );

/**
 * @brief Lock a mutex.
 *
 * @param[in] pMutex mbedtls mutex handle.
 * @return 0 on success
 */
int mbedtls_platform_mutex_lock( mbedtls_threading_mutex_t * pMutex );

/**
 * @brief Unlock a mutex.
 *
 * @param[in] pMutex mbedtls mutex handle.
 * @return 0 on success
 */
int mbedtls_platform_mutex_unlock( mbedtls_threading_mutex_t * pMutex );

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_FREERTOS_PORT_H */
