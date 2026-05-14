/*
 * Copyright (c) 2024, Texas Instruments Incorporated
 * All rights reserved.
 *
 * FreeRTOS Custom Configuration
 * This file contains custom FreeRTOS configuration that won't be overwritten by syscfg
 */

#ifndef FREERTOS_CONFIG_CUSTOM_H
#define FREERTOS_CONFIG_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Random number generation for Azure IoT backoff algorithm */
unsigned long uxRand( void );
#define configRAND32()    uxRand()

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_CUSTOM_H */
