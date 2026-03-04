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
 * @brief Client's private key.
 *
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
