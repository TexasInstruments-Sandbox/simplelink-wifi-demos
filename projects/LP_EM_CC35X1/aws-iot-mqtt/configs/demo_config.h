/*
 * AWS IoT Device SDK for Embedded C 202412.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"
#include "uart_term.h"

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
    #define LIBRARY_LOG_NAME    "AwsIoTDemo"
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
 * @brief Details of the MQTT broker to connect to.
 *
 * @note Your AWS IoT Core endpoint can be found in the AWS IoT console under
 * Settings/Custom Endpoint, or using the describe-endpoint API.
 *
 * #define AWS_IOT_ENDPOINT               "...insert here..."
 */

/**
 * @brief AWS IoT MQTT broker port number.
 *
 * In general, port 8883 is for secured MQTT connections.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name. When using port 8883, ALPN is not required.
 */
#ifndef AWS_MQTT_PORT
    #define AWS_MQTT_PORT    ( 8883 )
#endif

/**
 * @brief Path of the file containing the server's root CA certificate.
 *
 * This certificate is used to identify the AWS IoT server and is publicly
 * available. Refer to the AWS documentation available in the link below
 * https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs
 *
 * Amazon's root CA certificate is automatically downloaded to the certificates
 * directory from @ref https://www.amazontrust.com/repository/AmazonRootCA1.pem
 * using the CMake build system.
 *
 * @note This certificate should be PEM-encoded.
 * @note This path is relative from the demo binary created. Update
 * ROOT_CA_CERT_PATH to the absolute path if this demo is executed from elsewhere.
 */
#define democonfigROOT_CA_PEM                                              \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPP\r\n" \
    "QQQQRRRRSSSSTTTT\r\n"                                                 \
    "-----END CERTIFICATE-----\r\n"                                        \

/**
 * @brief Path of the file containing the client certificate.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This certificate should be PEM-encoded.
 *
 * #define CLIENT_CERT_PATH    "...insert here..."
 */

#ifndef democonfigCLIENT_CERTIFICATE_PEM
    #define democonfigCLIENT_CERTIFICATE_PEM    ""
#endif

/**
 * @brief Path of the file containing the client's private key.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This private key should be PEM-encoded.
 *
 * #define CLIENT_PRIVATE_KEY_PATH    "...insert here..."
 */

#ifndef democonfigCLIENT_PRIVATE_KEY_PEM
    #define democonfigCLIENT_PRIVATE_KEY_PEM    ""
#endif

/**
 * @brief The username value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client. If this config is defined,
 * the username and password will be used instead of the client certificate and
 * private key for client authentication.
 *
 * #define CLIENT_USERNAME    "...insert here..."
 */

/**
 * @brief The password value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client.
 *
 * #define CLIENT_PASSWORD    "...insert here..."
 */


/**
 * @brief AWS IoT Core device data endpoint.
 *
 * Found in: AWS IoT Console → Settings → Device data endpoint.
 * Format: @c "<prefix>-ats.iot.<region>.amazonaws.com"
 *
 * @note This value MUST be updated before flashing. The default is a
 *       placeholder that will fail DNS resolution.
 */
#define democonfigHOSTNAME          "x.x.x.amazonaws.com"

/**
 * @brief Set the stack size of the main demo task.
 *
 */
#define democonfigThingNamePrefix            "DevMac_"

/**
 * @brief Maximum byte length (including null terminator) of an AWS IoT
 *        Thing name string.
 */
#define AWS_IOT_MAX_THING_NAME    (64U)

/**
 * @brief MQTT keep-alive interval in seconds sent in the CONNECT packet.
 */
#define AWS_IOT_MQTT_KEEP_ALIVE_SEC       (60U)

/**
 * @brief Overall provisioning operation timeout in milliseconds.
 *
 * Applied independently to both the CreateCertificateFromCSR and
 * RegisterThing response waits.
 */
#define AWS_IOT_PROV_TIMEOUT_MS   (10000U)

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

#endif /* DEMO_CONFIG_H */
