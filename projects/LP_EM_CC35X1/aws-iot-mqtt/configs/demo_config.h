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
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n" \
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n" \
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n" \
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n" \
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n" \
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n" \
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n" \
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n" \
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n" \
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n" \
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n" \
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n" \
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n" \
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n" \
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n" \
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n" \
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n" \
    "rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"                                     \
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

 #define democonfigCLIENT_CERTIFICATE_PEM                                   \
 "-----BEGIN CERTIFICATE-----\r\n"                                      \
 "MIIDWTCCAkGgAwIBAgIUVPkA5skDAfSZkBtW4B8YSHyIEegwDQYJKoZIhvcNAQEL\r\n" \
 "BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\r\n" \
 "SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDUxNDE3NDQ0\r\n" \
 "NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\r\n" \
 "ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMemuVNNX7ZLCGH9zfuy\r\n" \
 "3VGjkHNCntksCbyI6x89ZjQ2n2MxeRZo/JyUK2ATk89mB2jHa3nCe1bIq139yLzW\r\n" \
 "CfNIVMsSWWrmVpOykWotvvzgir2b9Pj02NUQmVcxVHs4UGfbAllWOBYPl0OFCXcq\r\n" \
 "GjBTSFE8N3f3DPt5TlsXutPy5irssF8nyDfQUjHwtPQVw7B2IaiKZwPMQS7tTWyi\r\n" \
 "93km4s9Fd5elwMMVGX08QB6IbZvGxoQGmSRLxCsEOcGc53ESgem8kKlk9Kd8lH3Q\r\n" \
 "OvbnYXOBpdqIbp3k58XvgHlAD++Kr3tqLxRj7XMtJ4F4neZnjsk7WpfwX/t+8voJ\r\n" \
 "wTMCAwEAAaNgMF4wHwYDVR0jBBgwFoAUqp758B9+yC8E1INGx4xJHT4TDPMwHQYD\r\n" \
 "VR0OBBYEFDYMWyDGSSKG+yuamTyN2p9bXtQhMAwGA1UdEwEB/wQCMAAwDgYDVR0P\r\n" \
 "AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAf1CfrKBPMJJD/D2ZRKYQ5dBYQ\r\n" \
 "WFmgNG5Hgh97aKUTbiOAyHEvUNMAIqA3jbca+iNg8Qm/rJhZAdHEcPeVUZM7IXBC\r\n" \
 "9XNILawXz8QAzaDyeuSq1bJ6mrSdn8+uG1E9q/mm6FS2lQt+8jgSkE7Av4MuW5Zh\r\n" \
 "9UTQPaG0NuUbhvNLxfJisXrn4vmQx2a+bWAcPI1T/g3uyPm06kUoX8vKW031DudL\r\n" \
 "V6/7T+keTZbGilk4dS9GdsqFrh9HFJez3og7F15xz7XRQk25NZPbtkj2YFY5YenB\r\n" \
 "MacYQhNzcb8KotvPOLVoHK7pqwvLO3hYPxxgfVlpHopPpbSJfS1rnz0gxUut\r\n"     \
 "-----END CERTIFICATE-----\r\n"                                        \ 

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

 #define democonfigCLIENT_PRIVATE_KEY_PEM                               \
 "-----BEGIN RSA PRIVATE KEY-----\r\n"                                  \
 "MIIEpAIBAAKCAQEAx6a5U01ftksIYf3N+7LdUaOQc0Ke2SwJvIjrHz1mNDafYzF5\r\n" \
 "Fmj8nJQrYBOTz2YHaMdrecJ7VsirXf3IvNYJ80hUyxJZauZWk7KRai2+/OCKvZv0\r\n" \
 "+PTY1RCZVzFUezhQZ9sCWVY4Fg+XQ4UJdyoaMFNIUTw3d/cM+3lOWxe60/LmKuyw\r\n" \
 "XyfIN9BSMfC09BXDsHYhqIpnA8xBLu1NbKL3eSbiz0V3l6XAwxUZfTxAHohtm8bG\r\n" \
 "hAaZJEvEKwQ5wZzncRKB6byQqWT0p3yUfdA69udhc4Gl2ohuneTnxe+AeUAP74qv\r\n" \
 "e2ovFGPtcy0ngXid5meOyTtal/Bf+37y+gnBMwIDAQABAoIBAQCQME3Z5qCARBJK\r\n" \
 "ywVPiW8jfdBeHbghOhfSDMRaLHq6rNRRzDIaSDpgTvZAjgMLHzGsr5FkP1vaYlS5\r\n" \
 "LgcisiY/iHaMrrTban0OcEFrAJzVfslUhOTYQ+SxhCpqeVr9Hr+bMeWxZX5eGGZz\r\n" \
 "QgqDAGonio5I9QdOYaMDnylCypGIbbbWCCc3+KoTNtHVVJ2osBmhSCPaxIt/arLD\r\n" \
 "l06ttI6Jt8O+sO1mY42avH0WDtXEighLt+6OyvaorzIlfBa7jazrbAZwJqs7uwBh\r\n" \
 "BuBoF/QxpbJ1VPwr/1n7u2+lczktE5YL6y9kT5Rqej9iG0rwBMNRiSpsAn4GttYj\r\n" \
 "RW4ftMlhAoGBAOIcCf4Zzt4ogB8/CYSWDTm+CugE/fra2tOfMCJG/nHeJ5uZ29to\r\n" \
 "b4uIaMH/Rn606HEcSdfPagqliLayIKLKJSqMFJlLmboVbpKHFubGYNXP/D2QQMLl\r\n" \
 "qlmnJ4L2iPpFq2Zr54q8vq4qzSEu5+qHeGcpQdW63wJyVycFWo1Wd8ORAoGBAOIL\r\n" \
 "Se5jWwM7jqGvpKwumacscHyyVtY/KTkry9ip/iHYeJUWF6mBHyJmXqbjmHpL+FPq\r\n" \
 "P9xyefGcn84P6PLV7nTnaOpdIM1snNzvzctGfQSkGpcdZ9T9xdh0v/X8dK+vSklG\r\n" \
 "MAXQqNPl6bbcyuoWFwVDghJQsZXkRHJzPiPwQs6DAoGBAMsDt+khjP8lSBxGAiq8\r\n" \
 "e3V779jxGoWQ2WenB5XPPohImjF4jNHMTFLxEHYW2VnM3uMoLhkKD0Et7blz6B+h\r\n" \
 "9orkKV6WZZmRwqznhCWCutFfJDC2g586jKBgl/ZbmxNzWHjmq4eC/oXswi9oKS0H\r\n" \
 "o5Ckl4rqdW+B8ESF5w7+MxwxAoGAEHgwF81naTHir4cjoWP30AVd4MgBqbSKZV76\r\n" \
 "iDvCh2bFSl+Es9XzlccNqG02w9pbKooLwx0YI3F89z+TgnXx3NSrgT/tzunV+AcE\r\n" \
 "4IWvQDJQuafr08n1q1K+VcbiHZbQ+8vAXfwZAO9keu1VX37iiIClrn3wonIyRoB9\r\n" \
 "C5NK2s8CgYAUfOmVphrMJj3LZN1qJRLt4pYSVw3JYpwH0YEh/zuwxz6BeCcrf6Sx\r\n" \
 "xjkniCnkQXxl7IRCtMJ+eAP9ewsHK+khhdEc5+12Zq4Aq4y/5KqlbQh0rJ+zXCK3\r\n" \
 "bH4VSPbXIHY6MJ9eg7qTR/FxM2pvkq7C01GxFo+4gxzFBamOZgHxaA==\r\n"                                             \
 "-----END RSA PRIVATE KEY-----\r\n"                                    \ 

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
#define democonfigHOSTNAME          "ajk0b0cq50m2v-ats.iot.us-east-1.amazonaws.com"

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
