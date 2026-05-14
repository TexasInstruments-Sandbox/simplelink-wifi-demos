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
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n" \
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n" \
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n" \
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n" \
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n" \
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n" \
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n" \
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n" \
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n" \
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n" \
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n" \
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n" \
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n" \
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n" \
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n" \
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n" \
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n" \
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n" \
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"                             \
    "-----END CERTIFICATE-----\r\n"                                        \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n" \
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n" \
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n" \
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n" \
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n" \
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n" \
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n" \
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n" \
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n" \
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n" \
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n" \
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n" \
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n" \
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n" \
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n" \
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n" \
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n" \
    "MrY=\r\n"                                                             \
    "-----END CERTIFICATE-----\r\n"                                        \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "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\r\n" \
    "RA+GsCyRxj3qrg+E\r\n"                                                 \
    "-----END CERTIFICATE-----\r\n"

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
 "MIIDfTCCAmWgAwIBAgIRAIzDVWQCeMkBtdRt5JGFFugwDQYJKoZIhvcNAQELBQAw\r\n" \
 "ETEPMA0GA1UEAwwGb3NwcmV5MB4XDTI1MTAyMjEzNTg0N1oXDTI2MTAyMjEzNTg0\r\n" \
 "N1owbDELMAkGA1UEBhMCVVMxDjAMBgNVBAgMBVRleGFzMQ8wDQYDVQQKDAZvc3By\r\n" \
 "ZXkxDTALBgNVBAsMBHdpZmkxDzANBgNVBAMMBm9zcHJleTEcMBoGCSqGSIb3DQEJ\r\n" \
 "ARYNYi1saXU2QHRpLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n" \
 "AMJzcIsputGefzA3OKMMhNauCRt14lOEhj6OtAk1DYfUo38y3pNXBQBBTeXk5/aN\r\n" \
 "1lDyeXAbpGKcGd63+j4CT7qugJoj7rn6ydpsLa8hYkagMD1WdC8MPfDK+OAeP2JA\r\n" \
 "fM7U+KxBie2cgp3ZDXBZGxpp+Q9ruOzJngTsXN/e8Spj4Y/1hUjOgKk76IbC/H2J\r\n" \
 "2NKIaxY3v40z5l9Ns1I8YU6T4BAYOD51kzrPyRW5eq1mngIxBij3UH2Y7Fkll5Lj\r\n" \
 "h/CSHmGeQI7Z2FsMij1g5ciYFBVk1KScO74QTh4h0wjB38z+0cvTSMsttaObZv9n\r\n" \
 "Fb4IJ91Rkp9xdggqCGfF/nsCAwEAAaN1MHMwHQYDVR0OBBYEFCdQqbNWzC6m+BFq\r\n" \
 "NkgUn02XpsHeMAwGA1UdEwEB/wQCMAAwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDgYD\r\n" \
 "VR0PAQH/BAQDAgeAMB8GA1UdIwQYMBaAFHe8YzfJn8GvlM++BIH2G/z7uFt+MA0G\r\n" \
 "CSqGSIb3DQEBCwUAA4IBAQBMXu8fq5oblAWKmrh+uIkl9vT8r8W+i3IgadxIJWlU\r\n" \
 "+7AY3Zh6epi2GiMgBwlI7aSMsBgQaeqEECbZGQDzsqL6pd+TNuWNQQEG2LWAvsmy\r\n" \
 "7qgAdk2uOXzZLtZgOZouLQUZ7H+gmsrIo1ErfAgPGLtLwTkD/RJs+qOW5dn2BxBM\r\n" \
 "aK7ABdG+C9WloYAxHgHNacqfcSRQH3lh+behXoZFwS10UmJEn79c03rv7B9vgwKE\r\n" \
 "fA1Ls8L1wjiHXY6mP6H9LT1C1SO42lfOpJZFtjdU7ON8r/xLyvR/1nP4Vdb81vcl\r\n" \
 "faNTI0OVgzqWPMSppE0BhV68iNt6GICzCysA5QuHzcSr\r\n"                     \
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
 "-----BEGIN PRIVATE KEY-----\r\n"                                      \
 "MIIEugIBADANBgkqhkiG9w0BAQEFAASCBKQwggSgAgEAAoIBAQDCc3CLKbrRnn8w\r\n" \
 "NzijDITWrgkbdeJThIY+jrQJNQ2H1KN/Mt6TVwUAQU3l5Of2jdZQ8nlwG6RinBne\r\n" \
 "t/o+Ak+6roCaI+65+snabC2vIWJGoDA9VnQvDD3wyvjgHj9iQHzO1PisQYntnIKd\r\n" \
 "2Q1wWRsaafkPa7jsyZ4E7Fzf3vEqY+GP9YVIzoCpO+iGwvx9idjSiGsWN7+NM+Zf\r\n" \
 "TbNSPGFOk+AQGDg+dZM6z8kVuXqtZp4CMQYo91B9mOxZJZeS44fwkh5hnkCO2dhb\r\n" \
 "DIo9YOXImBQVZNSknDu+EE4eIdMIwd/M/tHL00jLLbWjm2b/ZxW+CCfdUZKfcXYI\r\n" \
 "Kghnxf57AgMBAAECgf8UghhclQofEZhYfXp7t9ZKzhAn1UcJh/CgqGxUjEPhD4wu\r\n" \
 "3i6bW5IrdLfCh2HvnwX7g5dLO58ax1vzIXRDFftStRTbLO8ArnKbls06q0qyKDLu\r\n" \
 "RS/7xk7CFqYo/QeyVRrUtaGEiWUCqn8cJshEESHKp2KdPb9DOgBwhH1HHzxQPAl6\r\n" \
 "FtxWh5GQWzrrQATUY84no7HLVKlQep6WBQpf4qH73XVBdOpHoIHOTsarxiAtLufy\r\n" \
 "x4R+rWBooE4j/AHdWuZccFpmKfXbVdDpAPVnBPPHAcScjXJvUhlWWTiP5ZLHnv7t\r\n" \
 "tfrbPBGvEe7NjWKH2sOfFXgqDT/AHj2tCedPxdkCgYEA5OahFXIAh+NSK/rDd29C\r\n" \
 "r2ilxnK2VP6kOU/OE3FOgNluFJkb3hcIRNuT3mLAm+26IG7JhposQvKNPa/k9EJ/\r\n" \
 "db1PTanKDK/aUKz06eNTF/n2etihn5l8oUIBi3M1nvvnWrUS8L9MIyxwYZSgnPMK\r\n" \
 "eypBJLxLGpMSX2DygW5Qna0CgYEA2Xi5iDtmj5c3iodx4x1dlgbalwuQDxtz29oH\r\n" \
 "5/4Tu8XCylHeSbCCSkSLu35ZnNkwctjB5UEYH7Dx8z41wTCCv47dOo0AM6zJvj4F\r\n" \
 "6FpDVHmvY/daWxbOsLC0IlR5JCd0WfTG0jBraMNYqpiaMY7VEVLZHTiHSoQmSANh\r\n" \
 "cLCswccCgYAP5hT6MFjpF1j1xQ0elpPDKzmYdw9DCAzQDkL6Bry1JmUG+Tt+SYtB\r\n" \
 "fR7gK2vFaNWsuwU0XDEG1WIKDtWQQa4ot2Vyt4BZMT2wrNK2DBwRZkNAdRuEwlxt\r\n" \
 "tu/0h6QJzuVa34jZP/BB4SxNGs2tGRR5SY4MxydQSTbVeR5e7xKaYQKBgDcuF72G\r\n" \
 "fPaQQwqGZZJgIB/yK0LKL0p2B8bQItNtt6ocetmFPmbqP5UCoYX4gFp5DX8GmuRF\r\n" \
 "yQ45gpravR8A5Rwf7uhtYJC8FhCMgkk7pfvoSGWQHMAiIF3a6EoRrw9KZ45vuocY\r\n" \
 "vBhAAN6AjBdpC40yvV5ZupJgx6ncGbxTR/J/AoGAY0OHortT0FKFMnNC1IaEiORx\r\n" \
 "Q40lGdcJrWnpXzUwtnDR05Wfshg08LwyiII44JYQMLO9hvqq+tsaIsyOOWowcV9h\r\n" \
 "sNqS12NZHr3U/KZy58XbLgWAIEHUYw7UyieWG7bwuzpubgftIygNaj1Wld0M0D85\r\n" \
 "utId+1lSpmSDKtxBl3c=\r\n"                                             \
 "-----END PRIVATE KEY-----\r\n"                                        \ 

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
#define democonfigHOSTNAME          "a2kcph6z19z4pz-ats.iot.us-west-2.amazonaws.com"

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
