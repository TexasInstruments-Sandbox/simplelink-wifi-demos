/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"
#include "uart_term.h"

/*
 * This plug-and-play model can be found at:
 * https://github.com/Azure/iot-plugandplay-models/blob/main/dtmi/com/example/thermostat-1.json
 * This Model ID is tightly tied to the code implementation in `sample_azure_iot_pnp_simulated_device.c`
 * If you intend to test a different Model ID, please provide the implementation of the model on your application.
 */
#define sampleazureiotMODEL_ID    "dtmi:com:example:Thermostat;1"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "../logging/logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "AzureIoTDemo"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/*
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging */
#ifndef SdkLog
    #define SdkLog( message )    UART_PRINT message
#endif

#include "../logging/logging_stack.h"

/************ End of logging configuration ****************/

/**
 * @brief Enable Device Provisioning
 *
 * @note To disable Device Provisioning undef this macro
 *
 */
// #define democonfigENABLE_DPS_SAMPLE

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Provisioning service endpoint.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#service-operations-endpoint
 *
 */
    #define democonfigENDPOINT           "global.azure-devices-provisioning.net"

/**
 * @brief Id scope of provisioning service.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#id-scope
 *
 */
    #define democonfigID_SCOPE           "0ne010214C2"

/**
 * @brief Registration Id of provisioning service
 *
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/azure/iot-dps/concepts-service#registration-id
 */
    #define democonfigREGISTRATION_ID    "device-01"

#endif /* democonfigENABLE_DPS_SAMPLE */

/**
 * @brief IoTHub device Id.
 *
 */
#define democonfigDEVICE_ID               "osprey"
/**
 * @brief IoTHub module Id.
 *
 * @note This is optional argument for IoTHub
 */
#define democonfigMODULE_ID               ""

/**
 * @brief IoTHub hostname.
 *
 */
#define democonfigHOSTNAME                "ospreyMX35.azure-devices.net"

/**
 * @brief Device symmetric key
 *
 */
// #define democonfigDEVICE_SYMMETRIC_KEY    "sQz8xXPw+Ziw7LloujPgAvl5AiudErDhxQe0WbtFx1s="

/**
 * @brief Client's X509 Certificate.
 *
 */
#define democonfigCLIENT_CERTIFICATE_PEM                                   \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPP\r\n" \
    "QQQQRRRRSSSSTTTT\r\n"                                                 \
    "-----END CERTIFICATE-----\r\n"                                        \

/**
 * @brief Client's private key.
 *
 */
#define democonfigCLIENT_PRIVATE_KEY_PEM                               \
"-----BEGIN PRIVATE KEY-----\r\n"                                      \
"AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPP\r\n" \
"QQQQRRRRSSSSTTTT\r\n"                                                 \
"-----END PRIVATE KEY-----\r\n"                                        \

/**
 * @brief Load the required certificates:
 *  - Baltimore Trusted Root CA
 *  - DigiCert Global Root G2
 *  - Microsoft RSA Root Certificate Authority 2017
 *
 * @warning Hard coding certificates is not recommended by Microsoft as a best
 * practice for production scenarios. Please see our document here for notes on best practices.
 * https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/main/docs/certificate-notice.md
 *
 */
#define democonfigROOT_CA_PEM                                              \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPP\r\n" \
    "QQQQRRRRSSSSTTTT\r\n"                                                 \
    "-----END CERTIFICATE-----\r\n"
/**
 * @brief Set the stack size of the main demo task.
 *
 */
#define democonfigDEMO_STACKSIZE             ( 2 * 1024U )

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE        ( 5 * 1024U )

/**
 * @brief IoTHub endpoint port.
 */
#define democonfigIOTHUB_PORT                ( 8883 )

/* 2^16 */
#define democonfigCHUNK_DOWNLOAD_SIZE        65536

#define democonfigADU_DEVICE_MANUFACTURER    "TI"
#define democonfigADU_DEVICE_MODEL           "CC35XX"
#define democonfigADU_UPDATE_PROVIDER        "TI-OTA"
#define democonfigADU_UPDATE_NAME            "NewCC35XXFW"
#define democonfigADU_UPDATE_VERSION         "1.0"
#define democonfigADU_UPDATE_NEW_VERSION     "1.1"

#ifndef configRAND32
    #include <stdlib.h>
    #define configRAND32()    ( rand() / RAND_MAX )
#endif

#endif /* DEMO_CONFIG_H */
