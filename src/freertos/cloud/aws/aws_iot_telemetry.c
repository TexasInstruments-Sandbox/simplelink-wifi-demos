/**
 * @file aws_iot_telemetry.c
 * @brief Periodic telemetry publisher for AWS IoT Core + Amazon Timestream.
 *
 * Establishes a mutual-TLS MQTT connection to AWS IoT Core using device
 * credentials, then periodically publishes a JSON telemetry payload to the topic
 *
 * An AWS IoT Rule routes matching messages to an Amazon Timestream table.
 *
 * @par MQTT topology
 * - No subscriptions — device only publishes.
 * - MQTT_ProcessLoop is called every 100mSec between publishes to service
 *   keepalive PINGRESPs and incoming PUBACKs for QoS 1 messages.
 *
 * @par Credential loading
 * - Device certificate: embedded as @c democonfigCLIENT_CERTIFICATE_PEM[] in demo_config.h
 * - Private key:        embedded as @c democonfigCLIENT_PRIVATE_KEY_PEM[] in demo_config.h
 * - Thing name:         read via @c AwsIotProv_GetThingName() (item 0x0104)
 * - AWS Root CA:        embedded as @c democonfigROOT_CA_PEM[] in demo_config.h
 */

/* Standard */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "backoff_algorithm.h"

/* Application */
#include "aws_iot_client_app.h"

/* AWS IoT service headers */
#include "demo_config.h"
#include "aws_iot_telemetry.h"

/* AWS IoT Hub */
#include "aws_iot_mqtt.h"

/* coreMQTT */
#include "core_mqtt.h"
#include "core_mqtt_serializer.h"
#include "core_mqtt_config_defaults.h"

/* transport */
#include "transport_tls_socket.h"
#include "transport_abstraction.h"

/* UART terminal */
#include "uart_term.h"

/* Driver Services */
#include "wlan_if.h"
#include <ti/drivers/I2C.h>

/* SNTP-backed wall-clock time (epoch seconds, set by sntpWrapper_updateDateTime) */
#include "date_time_service.h"

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleawsiotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleawsiotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleawsiotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleawsiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief The length of the outgoing publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for outgoing publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleOUTGOING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleOUTGOING_PUBLISH_RECORD_LEN            ( 15U )

/**
 * @brief The length of the incoming publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for incoming publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleINCOMING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleINCOMING_PUBLISH_RECORD_LEN            ( 15U )

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND    ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK      ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/**
 * @brief Defines configRAND32, used by the common sample modules.
 */
#define configRAND32()    ( rand() / RAND_MAX )

#define MAC_STR_LEN ( 16U )
#define MAC_2_STR(mac, str)								\
		( 													\
			sprintf(str, "%02x%02x%02x%02x%02x%02x",	\
					mac[0], mac[1], mac[2],					\
					mac[3], mac[4], mac[5])					\
		)

#define NUM_OF_SENSORS  (2)
#define TEMP_SENSOR_IDX (0)
#define ACC_SENSOR_IDX (1)
/* I2C temperature sensor addresses */
/* I2C temperature sensor target addresses */
#define TMP107_BASSENSORS_ADDR 0x48
/* Temperature result registers */
#define TMP107_RESULT_REG 0x0000

/* I2C accelerometer sensor addresses */
/* I2C accelerometer sensor target addresses */
#define BMA456_BASSENSORS_ADDR 0x18
/* Accelerometer configuration register */
#define BMA456_ACC_CONF_REG 0x0040
/* Accelerometer power control register */
#define BMA456_PWR_CTRL_REG 0x007D
/* Accelerometer chip ID register */
#define BMA456_CHIPID_REG 0x0000
/* Accelerometer x-axis register */
#define BMA456_ACC_X_REG 0x0013
/* Accelerometer y-axis register */
#define BMA456_ACC_Y_REG 0x0015
/* Accelerometer z-axis register */
#define BMA456_ACC_Z_REG 0x0017

/*
 * Data structure containing currently supported I2C TMP sensors.
 * Sensors are ordered by descending preference.
 */
static struct
{
    uint8_t address;
    uint8_t resultReg;
    char *id;
    uint8 status;
} sensors[NUM_OF_SENSORS] = {{TMP107_BASSENSORS_ADDR, TMP107_RESULT_REG, "TMP107", false},
                             {BMA456_BASSENSORS_ADDR, BMA456_CHIPID_REG, "BMA456", false}};

I2C_Handle i2c;

/* ---------------------------------------------------------------------------
 * Module state
 * ---------------------------------------------------------------------------*/

 /* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's SocketTransportParams_t) */
struct NetworkContext
{
    /* SocketTransportParams_t */
    void * pParams;
};

static NetworkCredentials_t xNetworkCredentials = { 0 };
static TransportInterface_t xTransport;
static NetworkContext_t xNetworkContext = { 0 };
static TlsTransportParams_t xTlsTransportParams = { 0 };
static MQTTContext_t mqttContext = { 0 };
/**
 * @brief Global entry time into the application to use as a reference timestamp
 * in the #prvGetTimeMs function. #prvGetTimeMs will always return the difference
 * between the current time and the global entry time. This will reduce the chances
 * of overflow for the 32 bit unsigned integer used for holding the timestamp.
 */
static uint32_t ulGlobalEntryTimeMs;

/**
 * @brief Array to track the outgoing publish records for outgoing publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pOutgoingPublishRecords[ mqttexampleOUTGOING_PUBLISH_RECORD_LEN ];

/**
 * @brief Array to track the incoming publish records for incoming publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pIncomingPublishRecords[ mqttexampleINCOMING_PUBLISH_RECORD_LEN ];

/*
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/* Telemetry topic: AwsTI/DevMac_AABBCCDDEEFF/telemetry */
static char pTopic[128];

static char pThingName[AWS_IOT_MAX_THING_NAME];

#if 0
static NetworkContext_t  s_netCtx;
static MQTTContext_t     s_mqttCtx;
static MQTTFixedBuffer_t s_mqttBuf;

/* Smaller MQTT buffer than provisioning: telemetry only exchanges tiny packets
 * (CONNECT/CONNACK, PUBLISH ~200 B payload, PUBACK, PINGREQ/PINGRESP). */
static uint8_t           s_mqttNetBuf[AWS_IOT_TELEMETRY_MQTT_BUFFER_SIZE];

/* QoS 1 publish-record arrays */
static MQTTPubAckInfo_t  s_outgoingPubRecords[2];
static MQTTPubAckInfo_t  s_incomingPubRecords[4];

#endif

/* Monotonically incrementing MQTT packet identifier */
static uint16_t s_packetId = 1U;

/* Optional callback for incoming MQTT PUBLISH packets (e.g. OTA job notifications) */
static void (*s_publishCb)(MQTTPublishInfo_t *) = NULL;
/** @brief Optional tick callback invoked every ~100 ms in AwsIotTelemetry_Run(). */
static int  (*s_tickCb)(void)                   = NULL;

/* ---------------------------------------------------------------------------
 * MQTT event callback
 * ---------------------------------------------------------------------------*/

/**
 * @brief Minimal MQTT event callback.
 *
 * The telemetry connection has no subscriptions, so the only expected
 * incoming packets are PINGRESPs (handled internally by coreMQTT) and
 * PUBACKs for QoS 1 publishes.  Any unexpected PUBLISH is logged.
 */
static void telemetry_event_callback(MQTTContext_t               *pCtx,
                                     MQTTPacketInfo_t             *pPacketInfo,
                                     MQTTDeserializedInfo_t       *pDeserializedInfo)
{
    MQTTStatus_t xResult = MQTTSuccess;
    uint8_t * pucPayload = NULL;
    size_t ulSize = 0;
    uint32_t ulTopicCount = 0U;
    MQTTSubAckStatus_t subAckStatus;

    (void)pCtx;

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH)
    {
        if (s_publishCb != NULL && pDeserializedInfo != NULL &&
            pDeserializedInfo->pPublishInfo != NULL)
        {
            s_publishCb(pDeserializedInfo->pPublishInfo);
        }
        else
        {
            UART_PRINT("[Telemetry] PUBLISH received (no handler registered)\r\n");
        }
    }

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_SUBACK)
    {
        /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
            * It contains the status code indicating server approval/rejection for the subscription to the single topic
            * requested. The SUBACK will be parsed to obtain the status code */

        xResult = AwsIoTMQTT_GetSubAckStatusCodes( pPacketInfo, &pucPayload, &ulSize );

        /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
            * from the event callback and non-NULL parameters. */
        configASSERT( xResult == MQTTSuccess );

        // Return code meanings (MQTT 3.1.1), MQTTSubAckStatus_t:
        // 0x00 = Max QoS 0 granted
        // 0x01 = Max QoS 1 granted
        // 0x02 = Max QoS 2 granted
        // 0x80 = Subscription failed

        for( ulTopicCount = 0; ulTopicCount < ulSize; ulTopicCount++ )
        {
            subAckStatus = (MQTTSubAckStatus_t)pucPayload[ulTopicCount];
            switch (subAckStatus) {
              case 0x00:
                UART_PRINT("[Telemetry] SUBACK received %d: Subscribed QoS 0\r\n", ulTopicCount);
                break;
              case 0x01:
                UART_PRINT("[Telemetry] SUBACK received %d: Subscribed QoS 1\r\n", ulTopicCount);
                break;
              case 0x02:
                UART_PRINT("[Telemetry] SUBACK received %d: Subscribed QoS 2\r\n", ulTopicCount);
                break;
              case 0x80:
                UART_PRINT("[Telemetry] SUBACK received %d: Subscription FAILED\r\n", ulTopicCount);
                break;
            }
        }
    }

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PUBACK)
    {
        UART_PRINT("[Telemetry] PUBACK received for packet Id %u\r\n", pDeserializedInfo->packetIdentifier);
    }

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_UNSUBACK)
    {
        UART_PRINT("[Telemetry] UNSUBACK received for packet Id %u\r\n", pDeserializedInfo->packetIdentifier);
    }

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PINGRESP)
    {
         /* Nothing to be done from application as library handles
        * PINGRESP with the use of MQTT_ProcessLoop API function. */
        UART_PRINT("[Telemetry] PINGRESP should not be handled by the application "
                       "callback when using MQTT_ProcessLoop.\n" );
    }
}

/* ---------------------------------------------------------------------------
 * Private API
 * ---------------------------------------------------------------------------*/
/*-----------------------------------------------------------*/
/**
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->xDisableSni = pdFALSE;

    pxNetworkCredentials->pucRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = strlen( democonfigROOT_CA_PEM ) +1;

    #ifdef democonfigCLIENT_CERTIFICATE_PEM
    pxNetworkCredentials->pucClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
    pxNetworkCredentials->xClientCertSize = strlen(democonfigCLIENT_CERTIFICATE_PEM) +1;
    pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
    pxNetworkCredentials->xPrivateKeySize = strlen(democonfigCLIENT_PRIVATE_KEY_PEM) +1;
    #endif

    return 0;
}

/*-----------------------------------------------------------*/

/**
 * @brief Connect to server with backoff retries.
 */
uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t port,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       sampleawsiotRETRY_BACKOFF_BASE_MS,
                                       sampleawsiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleawsiotRETRY_MAX_ATTEMPTS ); // just assigning values

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        UART_PRINT( "[Telemetry] Creating a TLS connection to %s:%lu.\r\n", pcHostName, port );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             sampleawsiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             sampleawsiotTRANSPORT_SEND_RECV_TIMEOUT_MS ); // open, set up, and connect via socket and then engage in TLS connection with handshakes

        if( xNetworkStatus != eTLSTransportSuccess )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                UART_PRINT( "[Telemetry] Connection to the IoT Hub failed, all attempts exhausted." );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                UART_PRINT( "[Telemetry] Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTimeMs = ( uint32_t ) ( ulTimeMs - ulGlobalEntryTimeMs );

    return ulTimeMs;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

AwsIotTelemetryStatus_t AwsIotTelemetry_Init(void)
{
    I2C_Params i2cParams;
    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[2];
    uint16_t sensorIdx;

    /* opens I2C interface for sensor readings which is uploaded to AWS as telemetry data */
    I2C_init();

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c               = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        UART_PRINT("[Telemetry] error Initializing I2C\n\r");
        return AWS_IOT_TELEMETRY_ERROR_INIT;
    }
    else
    {
        UART_PRINT("[Telemetry] I2C Initialized!\n\r");
    }

    /*
     * Determine if I2C sensor is present by querying known I2C
     * target addresses.
     */
    for (sensorIdx = 0; sensorIdx < NUM_OF_SENSORS; sensorIdx++)
    {
        i2cTransaction.targetAddress = sensors[sensorIdx].address;
        txBuffer[0]                  = sensors[sensorIdx].resultReg;

        if (I2C_transfer(i2c, &i2cTransaction))
        {
            UART_PRINT(   "[Telemetry] Detected %s sensor with target address 0x%x\n\r",
                          sensors[sensorIdx].id,
                          sensors[sensorIdx].address);
            sensors[sensorIdx].status = true;
        }
        else
        {
            UART_PRINT("[Telemetry] Failed to detect %s sensor\n\r", sensors[sensorIdx].id);
            sensors[sensorIdx].status = false;
            return AWS_IOT_TELEMETRY_ERROR_INIT;
        }
    }

    i2cTransaction.targetAddress = sensors[ACC_SENSOR_IDX].address;
    i2cTransaction.writeCount = 2;
    txBuffer[0] = BMA456_ACC_CONF_REG;
    txBuffer[1] = 0x17;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        UART_PRINT(   "[Telemetry] Accelerometer performance mode disabled\n\r");
    }

    txBuffer[0] = BMA456_PWR_CTRL_REG;
    txBuffer[1] = 0x4;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        UART_PRINT(   "[Telemetry] Accelerometer mode enabled\n\r");
    }

    return AWS_IOT_TELEMETRY_SUCCESS;
}

AwsIotTelemetryStatus_t AwsIotTelemetry_Connect(void)
{
    AwsIotTelemetryStatus_t  status  = AWS_IOT_TELEMETRY_ERROR_CREDENTIALS;
    uint32_t ulStatus;
    MQTTStatus_t xResult;
    MQTTConnectInfo_t connInfo;
    bool sessionPresent = false;
    WlanMacAddress_t wlanMacAddress;
    char macStr[MAC_STR_LEN];

    /* ---- Step 1: TLS layer init and connect ---- */
    ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials );

    xNetworkContext.pParams = &xTlsTransportParams;

     /* Attempt to establish TLS session with IoT Hub. If connection fails,
    * retry after a timeout. Timeout value will be exponentially increased
    * until  the maximum number of attempts are reached or the maximum timeout
    * value is reached. The function returns a failure status if the TCP
    * connection cannot be established to the IoT Hub after the configured
    * number of attempts. */
    ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) democonfigHOSTNAME,
                                                        democonfigIOTHUB_PORT,
                                                        &xNetworkCredentials, &xNetworkContext );
    if (ulStatus == 0)
    {
        UART_PRINT("[Telemetry] AwsIoT Connected to Server successfully.\r\n");
    }
    else
    {
        UART_PRINT("[Telemetry] AwsIoT Failed to Connect to Server. Status: %d\r\n", ulStatus);
    }

    /* ---- Step 2: MQTT init ---- */
    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pNetworkContext = &xNetworkContext;
    xTransport.send = TLS_Socket_Send;
    xTransport.recv = TLS_Socket_Recv;
    xTransport.writev = NULL;

    xResult = AwsIoTMQTT_Init( &mqttContext,
                                &xTransport,
                                prvGetTimeMs,
                                telemetry_event_callback,
                                ucMQTTMessageBuffer,
                                sizeof( ucMQTTMessageBuffer ) );

    if (xResult == MQTTSuccess)
    {
        UART_PRINT("[Telemetry] AwsIoTHub Client initialized successfully.\r\n");
    }
    else
    {
        UART_PRINT("[Telemetry] AwsIoTHub Failed to Connect to Client. Status: %d\r\n", xResult);
    }

     /* Build the telemetry topic string */
    wlanMacAddress.roleType = WLAN_ROLE_STA;
    Wlan_Get(WLAN_GET_MACADDRESS, &wlanMacAddress);
    MAC_2_STR(wlanMacAddress.pMacAddress, macStr);
    snprintf(pThingName, sizeof(pThingName), "%s%s", democonfigThingNamePrefix, macStr);
    snprintf(pTopic, sizeof(pTopic), "%s/%s/telemetry",
             AWS_IOT_TELEMETRY_TOPIC_PREFIX, pThingName);

    UART_PRINT("[Telemetry] Topic: %s\r\n", pTopic);	

    /* ---- Step 3: MQTT CONNECT ---- */
    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.cleanSession            = true;
    connInfo.pClientIdentifier       = pThingName;
    connInfo.clientIdentifierLength  = (uint16_t)strlen(pThingName);
    connInfo.keepAliveSeconds        = AWS_IOT_MQTT_KEEP_ALIVE_SEC;

    xResult = AwsIoTMQTT_Connect( &mqttContext,
		                                           &connInfo,
		                                           NULL,
		                                           AWS_IOT_PROV_TIMEOUT_MS,
		                                           &sessionPresent );

    if (xResult != MQTTSuccess)
    {
        UART_PRINT("[Telemetry] MQTT_Connect failed: %d\r\n", (int)xResult);
	 TLS_Socket_Disconnect( &xNetworkContext );
	 
        status = AWS_IOT_TELEMETRY_ERROR_CONNECT;
    }
    else
    {
    	UART_PRINT("[Telemetry] MQTT connected as: %s\r\n", pThingName);
		
    	status = AWS_IOT_TELEMETRY_SUCCESS;
    }

    return status;
}

AwsIotTelemetryStatus_t AwsIotTelemetry_Run(void)
{
    char     jsonBuf[AWS_IOT_TELEMETRY_JSON_MAX_LEN];
    uint32_t uptime_s;
    MQTTStatus_t pubRet;
    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[2];
    uint8_t rxBuffer[2];
    char tempStr[16];
    char accXStr[16];
    char accYStr[16];
    char accZStr[16];
    int16_t temperature;
    uint8_t accXYZ;

    for (;;)
    {
        /* ---- Build JSON payload ---- */
        uptime_s = prvGetTimeMs() / 1000U;

        /* Use epoch seconds (uint32_t) as the sort key.
         * - Avoids %llu which is unreliable on some embedded runtimes.
         * - uint32_t epoch seconds are valid until 2106.
         * - Falls back to uptime_s when SNTP has not yet synced (epoch < 2020). */
        uint32_t epoch_s = datetime_secondsGet();
        uint32_t sort_key = (epoch_s > 1577836800UL) ? epoch_s : uptime_s;

        /* device_id is included in the payload so the IoT Rule SQL can use
         * SELECT * without relying on clientId(), which returns "" when AWS
         * IoT Core has no MQTT client context (e.g., reboot edge cases). */

	    /* thing name and topic were built during AWS connect - no change during a session. */
        /* reading sensors and fill onto the outgoing message */
        tempStr[0]='\0';
        /*
        * read temperature sensor
        */
        if (true == sensors[TEMP_SENSOR_IDX].status)
        {
            i2cTransaction.targetAddress = sensors[TEMP_SENSOR_IDX].address;
            txBuffer[0] = sensors[TEMP_SENSOR_IDX].resultReg;
            i2cTransaction.readCount = 2;

            if (I2C_transfer(i2c, &i2cTransaction))
            {
                /*
                * Extract degrees C from the received data;
                * see TMP sensor datasheet
                */
                temperature = (rxBuffer[0]);

                sprintf(tempStr, "%d", temperature);
                UART_PRINT("[Telemetry] temperature is %d degC\n\r", tempStr);
            }
            else
            {
                UART_PRINT("[Telemetry] failed to read temperature sensor\n\r");
            }
        }

        if (true == sensors[ACC_SENSOR_IDX].status)
        {
            i2cTransaction.targetAddress = sensors[ACC_SENSOR_IDX].address;
            txBuffer[0] = BMA456_ACC_X_REG;
            i2cTransaction.readCount = 1;

            if (I2C_transfer(i2c, &i2cTransaction))
            {
                accXYZ = (rxBuffer[0]);

                sprintf(accXStr, "%d", accXYZ);
                UART_PRINT("[Telemetry] accelerometer X axis is %d\n\r", accXStr);
            }
            else
            {
                UART_PRINT("[Telemetry] failed to read accelerometer sensor\n\r");
            }

            txBuffer[0] = BMA456_ACC_Y_REG;
            if (I2C_transfer(i2c, &i2cTransaction))
            {
                accXYZ = (rxBuffer[0]);

                sprintf(accYStr, "%d", accXYZ);
                UART_PRINT("[Telemetry] accelerometer Y axis is %d\n\r", accXStr);
            }
            else
            {
                UART_PRINT("[Telemetry] failed to read accelerometer sensor\n\r");
            }

            txBuffer[0] = BMA456_ACC_Z_REG;
            if (I2C_transfer(i2c, &i2cTransaction))
            {
                accXYZ = (rxBuffer[0]);

                sprintf(accZStr, "%d", accXYZ);
                UART_PRINT("[Telemetry] accelerometer Z axis is %d\n\r", accXStr);
            }
            else
            {
                 UART_PRINT("[Telemetry] failed to read accelerometer sensor\n\r");
            }
        }
		
        snprintf(jsonBuf, sizeof(jsonBuf),
                 "{"
                 "\"device_id\":\"%s\","
                 "\"timestamp_s\":%lu,"
                 "\"temperature_c\":%s,"
                 "\"axis_X\":%s,"
                 "\"axis_Y\":%s,"
                 "\"axis_Z\":%s,"
                 "\"battery_mv\":3700,"
                 "\"sleep_pct\":40,"
                 "\"uptime_s\":%lu,"
                 "\"rssi_dbm\":-65,"
                 "\"app_version\":\"%s\""
                 "}",
                 pThingName,
                 (unsigned long)sort_key,
                 tempStr,
                 accXStr,
                 accYStr,
                 accZStr,
                 (unsigned long)uptime_s,
                 APPLICATION_VERSION);

        /* ---- Publish at QoS 1 ---- */
        MQTTPublishInfo_t pub;
        memset(&pub, 0, sizeof(pub));
        pub.qos             = MQTTQoS1;
        pub.retain          = false;
        pub.pTopicName      = pTopic;
        pub.topicNameLength = (uint16_t)strlen(pTopic);
        pub.pPayload        = jsonBuf;
        pub.payloadLength   = strlen(jsonBuf);

        s_packetId = AwsIoTMQTT_GetPacketId( &mqttContext );
        pubRet = MQTT_Publish(&mqttContext, &pub, s_packetId);
        if (s_packetId == 0U) s_packetId = 1U;   /* skip zero; coreMQTT rejects it */

        if (pubRet != MQTTSuccess)
        {
            UART_PRINT("[Telemetry] MQTT_Publish failed: %d\r\n", (int)pubRet);
            return AWS_IOT_TELEMETRY_ERROR_PUBLISH;
        }

        UART_PRINT("[Telemetry] Published: %s, s_packetId %d\r\n", jsonBuf, s_packetId);

        /* ---- Wait for next publish period ---- */
        /* Drive MQTT_ProcessLoop every ~100 ms to service PINGRESPs and PUBACKs.
         * Period is measured with Clock_GetTimeMs() so the loop is immune to how
         * long each MQTT_ProcessLoop call takes (bounded by SO_RCVTIMEO = 100 ms). */
        uint32_t period_start = prvGetTimeMs();
        while ((prvGetTimeMs() - period_start) < AWS_IOT_TELEMETRY_PERIOD_MS)
        {
            vTaskDelay(100U / portTICK_PERIOD_MS);

            /* Invoke the periodic tick callback (e.g. button polling).
             * A non-zero return requests an early stop. */
            if (s_tickCb != NULL && s_tickCb() != 0)
            {
                return AWS_IOT_TELEMETRY_STOPPED;
            }

            MQTTStatus_t loopRet = MQTT_ProcessLoop(&mqttContext);
            if (loopRet != MQTTSuccess)
            {
                UART_PRINT("[Telemetry] MQTT_ProcessLoop error: %d\r\n", (int)loopRet);
                return AWS_IOT_TELEMETRY_ERROR_PUBLISH;
            }
#if 0
            /* Check if the publish callback (OTA handler) signalled a pending job */
            if (AwsIotOta_IsUpdatePending())
            {
                UART_PRINT("[Telemetry] OTA job pending — pausing telemetry\r\n");
                return AWS_IOT_TELEMETRY_OTA_PENDING;
            }
#endif			
        }
    }
}

void AwsIotTelemetry_Disconnect(void)
{
    MQTT_Disconnect(&mqttContext);
    TLS_Socket_Disconnect( &xNetworkContext );
}

MQTTContext_t *AwsIotTelemetry_GetMqttCtx(void)
{
    return &mqttContext;
}

void AwsIotTelemetry_RegisterPublishCallback(void (*cb)(MQTTPublishInfo_t *))
{
    s_publishCb = cb;
}

void AwsIotTelemetry_RegisterTickCallback(int (*cb)(void))
{
    s_tickCb = cb;
}

/**
 * @brief Retrieve the provisioned AWS IoT Thing name.
 *
 */
const char *AwsIot_GetThingName(void)
{
    return pThingName;
}
