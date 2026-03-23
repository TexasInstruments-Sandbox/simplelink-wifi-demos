#include <aziot_client_app.h>
#include <stdlib.h>
#include "FreeRTOS.h"

/* Board Header files */
#include "ti_drivers_config.h"
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "osi_kernel.h"

/* Kernel includes. */
#include "task.h"

/* Example Header files */
// #include "wlan_cmd.h"
#include "wlan_if.h"
#include "wifi_if.h"
#include "tcpip_if.h"
#include "led_if.h"
#include "osi_kernel.h"
#include "uart_term.h"
#include "time.h"
#include "sntp_wrapper.h"

#import "dns_if.h"
#include "lwip/pbuf.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"

#include "lwip/sockets.h"

#include "mbedtls/build_info.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "test/certs.h"
#include "config-hsm.h"

#include "core_mqtt.h"
#include "core_mqtt_serializer.h"

// #include <string.h>

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

#include "configs/demo_config.h"
#include "azure_iot_result.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"
#include "azure_iot_mqtt.h"
#include "azure_sample_crypto.h"

//ERRORS
#include "errors.h"

#include "transport_tls_socket.h"
#include "transport_abstraction.h"
#include "transport_socket.h"

#include <ti/drivers/GPIO.h>

#define SNTP_SERVER_1   "216.239.35.0"
#define SNTP_SERVER_2   "216.239.35.4"
#define SNTP_SERVER_3   "129.6.15.28"

#define SERVER_PORT MQTT_CONNECTION_PORT_NUMBER
#define SERVER_NAME MQTT_CONNECTION_ADDRESS
#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

#define BSSID_ADDR              (6)

typedef struct ConnectCmd
{
    uint8_t                 bssid[BSSID_ADDR];
    /* Ap's SSID */
    uint8_t                 *ssid;
    /* Security parameters - Security Type and Password */
    WlanSecParams_t secParams;
}ConnectCmd_t;

#define BIT_x(x)                                        (1 << (x))
#define IS_BIT_SET(bit_field, bit_num)                  (((bit_field) & BIT_x(bit_num)) > 0)
#define CLEAR_BIT_IN_BITMAP(bit_field,bit_num)          { (bit_field) &= ~ BIT_x(bit_num) ; }
#define SET_BIT_IN_BITMAP(bit_field,bit_num)            { (bit_field) |=   BIT_x(bit_num) ; }

#define NET_IF_STA_BIT              (0)
#define NET_IF_AP_BIT               (1)
#define NET_IF_IS_UP                (2)
#define NET_IF_DEVICE_BIT           (4)

#define WLAN_REASON_DEAUTH_LEAVING 3
#define WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY 4
extern OsiSyncObj_t p2p_find_stopped_syncObj;
extern Bool_e g_wait_p2p_scan_complete;

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

#define MAX_THREAD_ENTRY    (4)

struct msgQueue
{
    int   event;
    char* payload;
};

uint32_t ActiveNetIfBitMap = 0x00;
uint32_t isIp = 0;
appControlBlock     app_CB;
static ip4addr_t gIp4Addr = 0, gIp4Mask = 0, gIp4GW = 0;
void *hWifiConn;
static bool s_is_connected_to_internet = false;
AzureIoTHubClient_t xAzureIoTHubClient;

static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
    ((void) level);

    mbedtls_fprintf((FILE *) ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE *) ctx);
}

/* a helper struct to ensure memory before/after fd_set is not touched */
typedef struct _xx
{
    u8_t buf1[8];
    fd_set readset;
    u8_t buf2[8];
    fd_set writeset;
    u8_t buf3[8];
    fd_set errset;
    u8_t buf4[8];
} fdsets;

#define INIT_FDSETS(sets) do { \
  memset((sets)->buf1, 0xab, 8); \
  memset((sets)->buf2, 0xab, 8); \
  memset((sets)->buf3, 0xab, 8); \
  memset((sets)->buf4, 0xab, 8); \
}while(0)


/*-----------------------------------------------------------*/
/* Default values for configs. */
#ifndef democonfigCLIENT_IDENTIFIER

/**
 * @brief The MQTT client identifier used in this example.  Each client identifier
 * must be unique so edit as required to ensure no two clients connecting to the
 * same broker use the same client identifier.
 *
 * @note Appending __TIME__ to the client id string will help to create a unique
 * client id every time an application binary is built. Only a single instance of
 * this application's compiled binary may be used at a time, since the client ID
 * will always be the same.
 */
    #define democonfigCLIENT_IDENTIFIER    "testClient"__TIME__
#endif

#ifndef democonfigMQTT_BROKER_PORT

/**
 * @brief The port to use for the demo.
 */
    #define democonfigMQTT_BROKER_PORT    ( 8883 )
#endif

/*-----------------------------------------------------------*/
/**
 * @brief The maximum number of retries for network operation with server.
 */
#define mqttexampleRETRY_MAX_ATTEMPTS                     ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define mqttexampleRETRY_MAX_BACKOFF_DELAY_MS             ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define mqttexampleRETRY_BACKOFF_BASE_MS                  ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define mqttexampleCONNACK_RECV_TIMEOUT_MS                ( 1000U )

/**
 * @brief The topic to subscribe and publish to in the example.
 *
 * The topic name starts with the client identifier to ensure that each demo
 * interacts with a unique topic name.
 */
#define mqttexampleTOPIC                                  democonfigCLIENT_IDENTIFIER "/example/topic"

/**
 * @brief The number of topic filters to subscribe.
 */
#define mqttexampleTOPIC_COUNT                            ( 1 )

/**
 * @brief The MQTT message published in this example.
 */
#define mqttexampleMESSAGE                                "Hello World!"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define mqttexampleDELAY_BETWEEN_DEMO_ITERATIONS_TICKS    ( pdMS_TO_TICKS( 10000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 * Refer to FreeRTOS-Plus/Demo/coreMQTT_Windows_Simulator/readme.txt for more details.
 */
#define mqttexamplePROCESS_LOOP_TIMEOUT_MS                ( 2000U )

/**
 * @brief Keep alive time reported to the broker while establishing
 * an MQTT connection.
 *
 * It is the responsibility of the Client to ensure that the interval between
 * Control Packets being sent does not exceed the this Keep Alive value. In the
 * absence of sending any other Control Packets, the Client MUST send a
 * PINGREQ Packet.
 */
#define mqttexampleKEEP_ALIVE_TIMEOUT_SECONDS             ( 60U ) // default 60

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define mqttexampleDELAY_BETWEEN_PUBLISHES_TICKS          ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define mqttexampleTRANSPORT_SEND_RECV_TIMEOUT_MS         ( 200U )

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
 * Provide default values for undefined configuration settings.
 */
#ifndef democonfigOS_NAME
    #define democonfigOS_NAME    "FreeRTOS"
#endif

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND    ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK      ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
/*************   Azure IOT defines  **************************/
/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleazureiotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 3 * 1000U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS                 ( 10 * 1000U )

/**
 * @brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                                 "Hello World : %d !"

/**
 * @brief  The content type of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define sampleazureiotMESSAGE_CONTENT_TYPE                    "text%2Fplain"

/**
 * @brief  The content encoding of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define sampleazureiotMESSAGE_CONTENT_ENCODING                "us-ascii"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotPROPERTY                                "{ \"PropertyIterationForCurrentConnection\": \"%d\" }"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 60000U ) )

#define sixtySeconds (pdMS_TO_TICKS(60000U))

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define sampleazureiotPROCESS_LOOP_TIMEOUT_MS                 ( 500U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define sampleazureiotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

static uint8_t ucPropertyBuffer[ 80 ];
static uint8_t ucScratchBuffer[ 128 ];


/**
 * @brief Defines configRAND32, used by the common sample modules.
 */
#define configRAND32()    ( rand() / RAND_MAX )

/**
 * @brief The function used to demonstrate the connection to Azure IOT via MQTT.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters );

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t ulPort,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext );

/*-----------------------------------------------------------*/
/**
 * @brief Cloud message callback handler
 */
static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    LogInfo( ( "Command payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, 200,
                                               NULL, 0 ) != eAzureIoTSuccess )
    {
        LogInfo( ( "Error sending command response\r\n" ) );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Property mesage callback handler
 */
static void prvHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogInfo( ( "Device property document GET received" ) );
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device property reported property response received" ) );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device property desired property received" ) );
            break;

        default:
            LogError( ( "Unknown property message" ) );
    }

    LogInfo( ( "Property document payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
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


typedef void  (*_SpawnEntryFunc_t)(void* pValue);
typedef struct SpawnThreadEntry_t
{
    int8_t                  id;                 // Index for socket process that running
    OsiThread_t             pThread;            // Thread control block
    OsiSyncObj_t            syncObj;            // sync object to kill the thread
    BOOLEAN                 bIsRunning;         // Flag to indicate the thread is running
    int32_t                 sock;               // saved the sock
    _SpawnEntryFunc_t       entryFunc;          // Thread function
    char*                   pName;              // Thread name
    void*                   pParam;             // Thread params
} SpawnThreadEntry_t;


SpawnThreadEntry_t gSpawThread[MAX_THREAD_ENTRY] = {0};

int32_t socket_ThreadDestroy(int id)
{
    struct timeval opt;
    opt.tv_sec = 0;
    opt.tv_usec = 0;
    int32_t ret;

    if(MAX_THREAD_ENTRY > id && 0 <= id && gSpawThread[id].bIsRunning)
    {
        // Change flag
        gSpawThread[id].bIsRunning = FALSE;

        // Force timeout
        lwip_setsockopt(gSpawThread[id].sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&opt, sizeof(opt));

        // Waiting for thread will finish main loop
        ret = osi_SyncObjWait(&gSpawThread[id].syncObj, OSI_WAIT_FOR_SECOND * 60);
        if(OSI_OK != ret)
        {
            // ASSERT_GENERAL(0);
            return ret;
        }

        // Delete the sync object
        ret = osi_SyncObjDelete(&gSpawThread[id].syncObj);
        if(OSI_OK != ret)
        {
            // ASSERT_GENERAL(0);
            return ret;
        }

        // Now the thread finish and we can delete the thread
        ret = osi_ThreadDelete(&gSpawThread[id].pThread);
        if(OSI_OK != ret)
        {
            // ASSERT_GENERAL(0);
            return ret;
        }

        // Remove all resources

        gSpawThread[id].entryFunc   = NULL;
        gSpawThread[id].pName       = NULL;
        gSpawThread[id].syncObj     = NULL;

        gSpawThread[id].id          = -1;

        if(gSpawThread[id].pParam)
        {
            os_free(gSpawThread[id].pParam);
        }
        gSpawThread[id].pParam      = NULL;

        return 0;
    }

    return OSI_OPERATION_FAILED;

}


void killAllProcess()
{
    int id;
    int ret;
    // Go over all the process
    for(id = 0; id < MAX_THREAD_ENTRY; id++)
    {
        // If process running
        if(gSpawThread[id].bIsRunning)
        {
            ret = socket_ThreadDestroy(id);
            if(ret != OSI_OK)
            {
                Report("\r\n[KILL ERROR] process in id %d is not running! call show_socket for see the running id's \n\r", id);
                SHOW_WARNING(WLAN_OSI_ERROR_BASE - ret, OS_ERROR_MSG);
            }
        }
    }
    return;
}


int lwip_mbedtls_ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
    int ret;
    // Get the socket from the context
    int sock = ((mbedtls_net_context *)ctx)->fd;
    // Receive data from the socket
    ret = lwip_recv(sock, buf, len, 0);
    if (ret < 0) {
        // Handle receive error
        return ret;
    }

    return ret;
}

int lwip_mbedtls_ssl_send(void *ctx, const unsigned char *buf, size_t len)
{
    int ret;
    // Get the socket from the context
    int sock = ((mbedtls_net_context *)ctx)->fd;
    // Send the data over the socket
    ret = lwip_send(sock, buf, len, 0);
    if (ret < 0) {
        // Handle send error
        return ret;
    }

    return ret;
}


bool xAzureSample_IsConnectedToInternet()
{
    return s_is_connected_to_internet;
}

//OSPREY_MX-38
#define HWREG(x)                                                              \
        (*((volatile unsigned long *)(x))) //TODO temporary need to be removed
#define ICACHE_BASE 0x41902000  //TODO temporary need to be removed, only for M3, M$ has different address


/*!
    \brief          Display application banner

    This routine shows how to get device information form the NWP.
    Also, it prints the PHY, MAC, NWP and Driver versions.

    \param          appName    -   points to a string representing
                                   application name.

    \param          appVersion -   points to a string representing
                                   application version number.

    \return         Upon successful completion, the function shall return 0.
                    In case of failure,
                    this function would return negative value.

    \sa             sl_DeviceGet, sl_NetCfgGet

*/
int32_t DisplayAppBanner(char* appName, char* appVersion)
{
    UART_PRINT("******************************************************************\r\n");
    UART_PRINT("***************** %-28s *******************\r\n", appName);
    UART_PRINT("***************** %-28s *******************\r\n", appVersion);
    UART_PRINT("******************************************************************\r\n");
    return(0);
}

typedef union
{
#if LWIP_IPV6
    sockaddr_in6 in6;       /* Socket info for Ipv6 */
#endif
#if LWIP_IPV4
    struct sockaddr_in in4; /* Socket info for Ipv4 */
#endif
} sockAddr_t;


typedef struct ConnParams
{
    uint32_t netconnFlags; /**< Enumerate connection type  */
    const char *serverAddr; /**< Server Address: URL or IP  */
    uint16_t port; /**< Port number of MQTT server */
} ConnParams_t;


/*-----------------------------------------------------------*/


/**
 * @brief Sends an MQTT Connect packet over the already connected TLS over TCP connection.
 *
 * @param[in, out] pxMQTTContext MQTT context pointer.
 * @param[in] xNetworkContext Network context.
 */
static void prvCreateMQTTConnectionWithBroker( MQTTContext_t * pxMQTTContext,
                                               NetworkContext_t * pxNetworkContext );

/**
 * @brief Function to update variable #xTopicFilterContext with status
 * information from Subscribe ACK. Called by the event callback after processing
 * an incoming SUBACK packet.
 *
 * @param[in] Server response to the subscription request.
 */
static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo );

/**
 * @brief Subscribes to the topic as specified in mqttexampleTOPIC at the top of
 * this file. In the case of a Subscribe ACK failure, then subscription is
 * retried using an exponential backoff strategy with jitter.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTSubscribeWithBackoffRetries( MQTTContext_t * pxMQTTContext );

/**
 * @brief Publishes a message mqttexampleMESSAGE on mqttexampleTOPIC topic.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTPublishToTopic( MQTTContext_t * pxMQTTContext );

/**
 * @brief Unsubscribes from the previously subscribed topic as specified
 * in mqttexampleTOPIC.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTUnsubscribeFromTopic( MQTTContext_t * pxMQTTContext );

/**
 * @brief The timer query function provided to the MQTT context.
 *
 * @return Time in milliseconds.
 */
static uint64_t prvGetTimeMs( void );

/**
 * @brief Process a response or ack to an MQTT request (PING, PUBLISH,
 * SUBSCRIBE or UNSUBSCRIBE). This function processes PINGRESP, PUBACK,
 * SUBACK, and UNSUBACK.
 *
 * @param[in] pxIncomingPacket is a pointer to structure containing deserialized
 * MQTT response.
 * @param[in] usPacketId is the packet identifier from the ack received.
 */
static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId );

/**
 * @brief Process incoming Publish message.
 *
 * @param[in] pxPublishInfo is a pointer to structure containing deserialized
 * Publish message.
 */
static void prvMQTTProcessIncomingPublish( MQTTPublishInfo_t * pxPublishInfo );

/**
 * @brief The application callback function for getting the incoming publishes,
 * incoming acks, and ping responses reported from the MQTT library.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 * @param[in] pxPacketInfo Packet Info pointer for the incoming packet.
 * @param[in] pxDeserializedInfo Deserialized information from the incoming packet.
 */
static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo );

/**
 * @brief Call #MQTT_ProcessLoop in a loop for the duration of a timeout or
 * #MQTT_ProcessLoop returns a failure.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] ulTimeoutMs Duration to call #MQTT_ProcessLoop for.
 *
 * @return Returns the return value of the last call to #MQTT_ProcessLoop.
 */
static MQTTStatus_t prvProcessLoopWithTimeout( MQTTContext_t * pMqttContext,
                                               uint32_t ulTimeoutMs );

/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Global entry time into the application to use as a reference timestamp
 * in the #prvGetTimeMs function. #prvGetTimeMs will always return the difference
 * between the current time and the global entry time. This will reduce the chances
 * of overflow for the 32 bit unsigned integer used for holding the timestamp.
 */
static uint32_t ulGlobalEntryTimeMs;

/**
 * @brief Packet Identifier generated when Publish request was sent to the broker;
 * it is used to match received Publish ACK to the transmitted Publish packet.
 */
static uint16_t usPublishPacketIdentifier;

/**
 * @brief Packet Identifier generated when Subscribe request was sent to the broker;
 * it is used to match received Subscribe ACK to the transmitted Subscribe packet.
 */
static uint16_t usSubscribePacketIdentifier;

/**
 * @brief Packet Identifier generated when Unsubscribe request was sent to the broker;
 * it is used to match received Unsubscribe response to the transmitted Unsubscribe
 * request.
 */
static uint16_t usUnsubscribePacketIdentifier;

/**
 * @brief A pair containing a topic filter and its SUBACK status.
 */
typedef struct topicFilterContext
{
    const char * pcTopicFilter;
    MQTTSubAckStatus_t xSubAckStatus;
} topicFilterContext_t;

/**
 * @brief An array containing the context of a SUBACK; the SUBACK status
 * of a filter is updated when the event callback processes a SUBACK.
 */
static topicFilterContext_t xTopicFilterContext[ mqttexampleTOPIC_COUNT ] =
{
    { mqttexampleTOPIC, MQTTSubAckFailure }
};


/** @brief Static buffer used to hold MQTT messages being sent and received. */
static MQTTFixedBuffer_t xBuffer =
{
    ucSharedBuffer,
    democonfigNETWORK_BUFFER_SIZE
};

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



static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo )
{
    MQTTStatus_t xResult = MQTTSuccess;
    uint8_t * pucPayload = NULL;
    size_t ulSize = 0;
    uint32_t ulTopicCount = 0U;

    xResult = MQTT_GetSubAckStatusCodes( pxPacketInfo, &pucPayload, &ulSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    configASSERT( xResult == MQTTSuccess );

    for( ulTopicCount = 0; ulTopicCount < ulSize; ulTopicCount++ )
    {
        xTopicFilterContext[ ulTopicCount ].xSubAckStatus = pucPayload[ ulTopicCount ];
    }
}
/*-----------------------------------------------------------*/

static void prvMQTTSubscribeWithBackoffRetries( MQTTContext_t * pxMQTTContext )
{
    MQTTStatus_t xResult = MQTTSuccess;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xRetryParams;
    uint16_t usNextRetryBackOff = 0U;
    MQTTSubscribeInfo_t xMQTTSubscription[ mqttexampleTOPIC_COUNT ];
    bool xFailedSubscribeToTopic = false;
    uint32_t ulTopicCount = 0U;

    /* Some fields not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );

    /* Get a unique packet id. */
    usSubscribePacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Subscribe to the mqttexampleTOPIC topic filter. This example subscribes to
     * only one topic and uses QoS1. */
    xMQTTSubscription[ 0 ].qos = MQTTQoS1;
    xMQTTSubscription[ 0 ].pTopicFilter = mqttexampleTOPIC;
    xMQTTSubscription[ 0 ].topicFilterLength = ( uint16_t ) strlen( mqttexampleTOPIC );

    /* Initialize context for backoff retry attempts if SUBSCRIBE request fails. */
    BackoffAlgorithm_InitializeParams( &xRetryParams,
                                       mqttexampleRETRY_BACKOFF_BASE_MS,
                                       mqttexampleRETRY_MAX_BACKOFF_DELAY_MS,
                                       mqttexampleRETRY_MAX_ATTEMPTS );

    do
    {
        /* The client is now connected to the broker. Subscribe to the topic
         * as specified in mqttexampleTOPIC at the top of this file by sending a
         * subscribe packet then waiting for a subscribe acknowledgment (SUBACK).
         * This client will then publish to the same topic it subscribed to, so it
         * will expect all the messages it sends to the broker to be sent back to it
         * from the broker. This demo uses QOS0 in Subscribe, therefore, the Publish
         * messages received from the broker will have QOS0. */
        LogInfo( ( "Attempt to subscribe to the MQTT topic %s.\r\n", mqttexampleTOPIC ) );
        xResult = MQTT_Subscribe( pxMQTTContext,
                                  xMQTTSubscription,
                                  sizeof( xMQTTSubscription ) / sizeof( MQTTSubscribeInfo_t ),
                                  usSubscribePacketIdentifier );
        configASSERT( xResult == MQTTSuccess );

        LogInfo( ( "SUBSCRIBE sent for topic %s to broker.\n\n", mqttexampleTOPIC ) );

        /* Process incoming packet from the broker. After sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Therefore,
         * call generic incoming packet processing function. Since this demo is
         * subscribing to the topic to which no one is publishing, probability of
         * receiving Publish message before subscribe ack is zero; but application
         * must be ready to receive any packet.  This demo uses the generic packet
         * processing function everywhere to highlight this fact. */
        xResult = prvProcessLoopWithTimeout( pxMQTTContext, mqttexamplePROCESS_LOOP_TIMEOUT_MS );
        configASSERT( xResult == MQTTSuccess );

        /* Reset flag before checking suback responses. */
        xFailedSubscribeToTopic = false;

        /* Check if recent subscription request has been rejected. #xTopicFilterContext is updated
         * in the event callback to reflect the status of the SUBACK sent by the broker. It represents
         * either the QoS level granted by the server upon subscription, or acknowledgement of
         * server rejection of the subscription request. */
        for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
        {
            if( xTopicFilterContext[ ulTopicCount ].xSubAckStatus == MQTTSubAckFailure )
            {
                xFailedSubscribeToTopic = true;

                /* Generate a random number and calculate backoff value (in milliseconds) for
                 * the next connection retry.
                 * Note: It is recommended to seed the random number generator with a device-specific
                 * entropy source so that possibility of multiple devices retrying failed network operations
                 * at similar intervals can be avoided. */
                xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xRetryParams, uxRand(), &usNextRetryBackOff );

                if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
                {
                    LogError( ( "Server rejected subscription request. All retry attempts have exhausted. Topic=%s",
                                xTopicFilterContext[ ulTopicCount ].pcTopicFilter ) );
                }
                else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
                {
                    LogWarn( ( "Server rejected subscription request. Attempting to re-subscribe to topic %s.",
                               xTopicFilterContext[ ulTopicCount ].pcTopicFilter ) );
                    /* Backoff before the next re-subscribe attempt. */
                    vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
                }

                break;
            }
        }

        configASSERT( xBackoffAlgStatus != BackoffAlgorithmRetriesExhausted );
    } while( ( xFailedSubscribeToTopic == true ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );
}
/*-----------------------------------------------------------*/

static void prvMQTTPublishToTopic( MQTTContext_t * pxMQTTContext )
{
    MQTTStatus_t xResult;
    MQTTPublishInfo_t xMQTTPublishInfo;

    /***
     * For readability, error handling in this function is restricted to the use of
     * asserts().
     ***/

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = MQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = mqttexampleTOPIC;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) strlen( mqttexampleTOPIC );
    xMQTTPublishInfo.pPayload = mqttexampleMESSAGE;
    xMQTTPublishInfo.payloadLength = strlen( mqttexampleMESSAGE );

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    xResult = MQTT_Publish( pxMQTTContext, &xMQTTPublishInfo, usPublishPacketIdentifier );

    configASSERT( xResult == MQTTSuccess );
}
/*-----------------------------------------------------------*/

static void prvMQTTUnsubscribeFromTopic( MQTTContext_t * pxMQTTContext )
{
    MQTTStatus_t xResult;
    MQTTSubscribeInfo_t xMQTTSubscription[ mqttexampleTOPIC_COUNT ];

    /* Some fields not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );

    /* Get a unique packet id. */
    usSubscribePacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Subscribe to the mqttexampleTOPIC topic filter. This example subscribes to
     * only one topic and uses QoS1. */
    xMQTTSubscription[ 0 ].qos = MQTTQoS1;
    xMQTTSubscription[ 0 ].pTopicFilter = mqttexampleTOPIC;
    xMQTTSubscription[ 0 ].topicFilterLength = ( uint16_t ) strlen( mqttexampleTOPIC );

    /* Get next unique packet identifier. */
    usUnsubscribePacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Send UNSUBSCRIBE packet. */
    xResult = MQTT_Unsubscribe( pxMQTTContext,
                                xMQTTSubscription,
                                sizeof( xMQTTSubscription ) / sizeof( MQTTSubscribeInfo_t ),
                                usUnsubscribePacketIdentifier );

    configASSERT( xResult == MQTTSuccess );
}
/*-----------------------------------------------------------*/

static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    uint32_t ulTopicCount = 0U;

    switch( pxIncomingPacket->type )
    {
        case MQTT_PACKET_TYPE_PUBACK:
            LogInfo( ( "PUBACK received for packet Id %u.\r\n", usPacketId ) );
            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usPublishPacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_SUBACK:

            /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
             * It contains the status code indicating server approval/rejection for the subscription to the single topic
             * requested. The SUBACK will be parsed to obtain the status code, and this status code will be stored in global
             * variable #xTopicFilterContext. */
            prvUpdateSubAckStatus( pxIncomingPacket );

            for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
            {
                if( xTopicFilterContext[ ulTopicCount ].xSubAckStatus != MQTTSubAckFailure )
                {
                    LogInfo( ( "Subscribed to the topic %s with maximum QoS %u.\r\n",
                               xTopicFilterContext[ ulTopicCount ].pcTopicFilter,
                               xTopicFilterContext[ ulTopicCount ].xSubAckStatus ) );
                }
            }

            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usSubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_UNSUBACK:
            LogInfo( ( "Unsubscribed from the topic %s.\r\n", mqttexampleTOPIC ) );
            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usUnsubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_PINGRESP:

            /* Nothing to be done from application as library handles
             * PINGRESP with the use of MQTT_ProcessLoop API function. */
            LogWarn( ( "PINGRESP should not be handled by the application "
                       "callback when using MQTT_ProcessLoop.\n" ) );
            break;

        /* Any other packet type is invalid. */
        default:
            LogWarn( ( "prvMQTTProcessResponse() called with unknown packet type:(%02X).\r\n",
                       pxIncomingPacket->type ) );
    }
}

/*-----------------------------------------------------------*/

static void prvMQTTProcessIncomingPublish( MQTTPublishInfo_t * pxPublishInfo )
{
    configASSERT( pxPublishInfo != NULL );

    /* Process incoming Publish. */
    LogInfo( ( "Incoming QoS : %d\n", pxPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    if( ( pxPublishInfo->topicNameLength == strlen( mqttexampleTOPIC ) ) &&
        ( 0 == strncmp( mqttexampleTOPIC, pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength ) ) )
    {
        LogInfo( ( "\r\nIncoming Publish Topic Name: %.*s matches subscribed topic.\r\n"
                   "Incoming Publish Message : %.*s\r\n",
                   pxPublishInfo->topicNameLength,
                   pxPublishInfo->pTopicName,
                   pxPublishInfo->payloadLength,
                   pxPublishInfo->pPayload ) );
    }
    else
    {
        LogInfo( ( "Incoming Publish Topic Name: %.*s does not match subscribed topic.\r\n",
                   pxPublishInfo->topicNameLength,
                   pxPublishInfo->pTopicName ) );
    }
}

/*-----------------------------------------------------------*/

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* The MQTT context is not used for this demo. */
    ( void ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxDeserializedInfo->pPublishInfo );
    }
    else
    {
        prvMQTTProcessResponse( pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
}

/*-----------------------------------------------------------*/
static uint64_t prvGetTimeMs( void )
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

/*-----------------------------------------------------------*/

static MQTTStatus_t prvProcessLoopWithTimeout( MQTTContext_t * pMqttContext,
                                               uint32_t ulTimeoutMs )
{
    uint32_t ulMqttProcessLoopTimeoutTime;
    uint32_t ulCurrentTime;

    MQTTStatus_t eMqttStatus = MQTTSuccess;

    ulCurrentTime = pMqttContext->getTime();
    ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeoutMs;

    /* Call MQTT_ProcessLoop multiple times a timeout happens, or
     * MQTT_ProcessLoop fails. */
    while( ( ulCurrentTime < ulMqttProcessLoopTimeoutTime ) &&
           ( eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes ) )
    {
        eMqttStatus = MQTT_ProcessLoop( pMqttContext );
        ulCurrentTime = pMqttContext->getTime();
    }

    if( eMqttStatus == MQTTNeedMoreBytes )
    {
        eMqttStatus = MQTTSuccess;
    }

    return eMqttStatus;
}

/*-----------------------------------------------------------*/




/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's SocketTransportParams_t) */
struct NetworkContext
{
    /* SocketTransportParams_t */
    void * pParams;
};

static char *IP4ToStr(ip4addr_t ipAddress)
{
    static char ip4str[16];
    uint8_t *pU8 = (uint8_t*)&ipAddress;
    sprintf(ip4str, "%d.%d.%d.%d", pU8[0], pU8[1], pU8[2], pU8[3]);
    return ip4str;
}

static void OnWifiEvent(WifiConnStatus_e status, void *params)
{
    ConnectCmd_t ConnectParams;
    int32_t             RetVal = -1;

    switch (status)
    {
        case WIFI_STATUS_CONNECTED_IP:
        {
            TCPIP_IF_getIp4Addr(params, &gIp4Addr, &gIp4Mask, &gIp4GW);
            if(gIp4Addr)
            {
                UART_PRINT("OnWifiEvent(CONNECTED_IP): addr=%s\r\n", IP4ToStr(gIp4Addr));
                UART_PRINT("                           mask=%s\r\n", IP4ToStr(gIp4Mask));
                UART_PRINT("                           gw=%s\r\n", IP4ToStr(gIp4GW));
            }
            else
            {
                UART_PRINT("OnWifiEvent(CONNECTED_IP): IPv6 only\r\n");
            }
            s_is_connected_to_internet = true;
        }
        break;

        case WIFI_STATUS_DISCONNECTED:
            UART_PRINT("OnWifiEvent - Disconnected (%d)\r\n", status);
            s_is_connected_to_internet = false;
            break;
        default:
            UART_PRINT("OnWifiEvent(%d)\r\n", status);
            break;
    }
}


/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE
/**
 * @brief Get IoT Hub endpoint and device Id info, when Provisioning service is used.
 *   This function will block for Provisioning service for result or return failure.
 */
uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                       uint8_t ** ppucIothubHostname,
                                       uint32_t * pulIothubHostnameLength,
                                       uint8_t ** ppucIothubDeviceId,
                                       uint32_t * pulIothubDeviceIdLength )
{
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    AzureIoTTransportInterface_t xTransport;
    uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
    uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );
    uint32_t ulStatus;

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        ulStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                         pXNetworkCredentials, &xNetworkContext );
        if (ulStatus == 0)
        {

        }
        configASSERT( ulStatus == 0 );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;
	 xTransport.xWritev = NULL;

        #ifdef democonfigUSE_HSM

            /* Redefine the democonfigREGISTRATION_ID macro using registration ID
             * generated dynamically using the HSM */

            /* We use a pointer instead of a buffer so that the getRegistrationId
             * function can allocate the necessary memory depending on the HSM */
            char * registration_id = NULL;
            ulStatus = getRegistrationId( &registration_id );
            configASSERT( ulStatus == 0 );
#undef democonfigREGISTRATION_ID
        #define democonfigREGISTRATION_ID    registration_id
        #endif

        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   #ifdef democonfigUSE_HSM
                                                       strlen( democonfigREGISTRATION_ID ),
                                                   #else
                                                       sizeof( democonfigREGISTRATION_ID ) - 1,
                                                   #endif
                                                   NULL, ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                   prvGetTimeMs,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                  sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                  Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigDEVICE_SYMMETRIC_KEY */

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTErrorPending );

        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                              ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                              ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
        configASSERT( xResult == eAzureIoTSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        *ppucIothubHostname = ucSampleIotHubHostname;
        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

        return 0;
    }

#endif /* democonfigENABLE_DPS_SAMPLE */

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
                                       sampleazureiotRETRY_BACKOFF_BASE_MS,
                                       sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleazureiotRETRY_MAX_ATTEMPTS ); // just assigning values

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        LogInfo( ( "Creating a TLS connection to %s:%lu.\r\n", pcHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS ); // open, set up, and connect via socket and then engage in TLS connection with handshakes

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
                LogError( ( "Connection to the IoT Hub failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
void prvAzureDemoTask( void * pvParameters )
{
    int lPublishCount = 0;
    uint32_t ulScratchBufferLength = 0U;
    const int lMaxPublishCount = 1;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTMessageProperties_t xPropertyBag;
    bool xSessionPresent;

    uint8_t * pucIotHubHostname = ( uint8_t * ) democonfigHOSTNAME;
    uint8_t * pucIotHubDeviceId = ( uint8_t * ) democonfigDEVICE_ID;
    uint32_t pulIothubHostnameLength = sizeof( democonfigHOSTNAME ) - 1;
    uint32_t pulIothubDeviceIdLength = sizeof( democonfigDEVICE_ID ) - 1;

    ( void ) pvParameters;

    /* Initialize Azure IoT Middleware.  */
    xResult = AzureIoT_Init();
    if (xResult == eAzureIoTSuccess)
    {
        UART_PRINT("AzureIoT Initialized successfully.\r\n");

        ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials );

        #ifdef democonfigENABLE_DPS_SAMPLE
            /* Run DPS.  */
            if( ( ulStatus = prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname,
                                               &pulIothubHostnameLength, &pucIotHubDeviceId,
                                               &pulIothubDeviceIdLength ) ) != 0 )
            {
                LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ( uint16_t ) ulStatus ) );
                return;
            }
        #endif /* democonfigENABLE_DPS_SAMPLE */

        xNetworkContext.pParams = &xTlsTransportParams;

        for( ; ; )
        {
            if( xAzureSample_IsConnectedToInternet() )
            {
                /* Attempt to establish TLS session with IoT Hub. If connection fails,
                 * retry after a timeout. Timeout value will be exponentially increased
                 * until  the maximum number of attempts are reached or the maximum timeout
                 * value is reached. The function returns a failure status if the TCP
                 * connection cannot be established to the IoT Hub after the configured
                 * number of attempts. */
                ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                                 democonfigIOTHUB_PORT,
                                                                 &xNetworkCredentials, &xNetworkContext );
                if (ulStatus == 0)
                {
                    UART_PRINT("AzureIoT Connected to Server successfully.\r\n");
                }
                else
                {
                    UART_PRINT("AzureIoT Failed to Connect to Server. Status: %d\r\n", ulStatus);
                }

                /* Fill in Transport Interface send and receive function pointers. */
                xTransport.pxNetworkContext = &xNetworkContext;
                xTransport.xSend = TLS_Socket_Send;
                xTransport.xRecv = TLS_Socket_Recv;
		  xTransport.xWritev = NULL;

                /* Init IoT Hub option */
                xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
                if (xResult == eAzureIoTSuccess)
                {
                    UART_PRINT("AzureIoTHub Client Options initialized successfully.\r\n");
                }
                else
                {
                    UART_PRINT("AzureIoTHub Failed to Initialize Options. Status: %d\r\n", xResult);
                }

                configASSERT( xResult == eAzureIoTSuccess );

                xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
                xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;

                xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                                  pucIotHubHostname, pulIothubHostnameLength,
                                                  pucIotHubDeviceId, pulIothubDeviceIdLength,
                                                  &xHubOptions,
                                                  ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                  prvGetTimeMs,
                                                  &xTransport );
                if (xResult == eAzureIoTSuccess)
                {
                    UART_PRINT("AzureIoTHub Client initialized successfully.\r\n");
                }
                else
                {
                    UART_PRINT("AzureIoTHub Failed to Connect to Client. Status: %d\r\n", xResult);
                }
                configASSERT( xResult == eAzureIoTSuccess );

                #ifdef democonfigDEVICE_SYMMETRIC_KEY
                    xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                                 ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                 sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                 Crypto_HMAC );
                    configASSERT( xResult == eAzureIoTSuccess );
                #endif /* democonfigDEVICE_SYMMETRIC_KEY */

                /* Sends an MQTT Connect packet over the already established TLS connection,
                 * and waits for connection acknowledgment (CONNACK) packet. */
                LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

                xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                     false, &xSessionPresent,
                                                     sampleazureiotCONNACK_RECV_TIMEOUT_MS );
                if (xResult == eAzureIoTSuccess)
                {
                    LogInfo( ("AzureIoTHub Client Connected successfully.\r\n") );
                }
                else
                {
                    LogInfo( ("AzureIoTHub Failed to Connect. Status: %d\r\n", xResult) );
                }
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                           &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                              &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandlePropertiesMessage,
                                                                 &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Get property document after initial connection */
                xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Create a bag of properties for the telemetry */
                xResult = AzureIoTMessage_PropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( ucPropertyBuffer ) );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Sending a default property (Content-Type). */
                xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                            ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE ) - 1,
                                                            ( uint8_t * ) sampleazureiotMESSAGE_CONTENT_TYPE, sizeof( sampleazureiotMESSAGE_CONTENT_TYPE ) - 1 );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Sending a default property (Content-Encoding). */
                xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                            ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING ) - 1,
                                                            ( uint8_t * ) sampleazureiotMESSAGE_CONTENT_ENCODING, sizeof( sampleazureiotMESSAGE_CONTENT_ENCODING ) - 1 );
                configASSERT( xResult == eAzureIoTSuccess );

                /* How to send an user-defined custom property. */
                xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                            ( uint8_t * ) "value", sizeof( "value" ) - 1 );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Publish messages with QoS1, send and process Keep alive messages. */
                for( lPublishCount = 0;
                     lPublishCount < lMaxPublishCount && xAzureSample_IsConnectedToInternet();
                     lPublishCount++ )
                {
                    ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                      sampleazureiotMESSAGE, lPublishCount );
                    xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                               ucScratchBuffer, ulScratchBufferLength,
                                                               &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
                    configASSERT( xResult == eAzureIoTSuccess );

                    LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
                    xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                             sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
                    configASSERT( xResult == eAzureIoTSuccess );

                    // if( lPublishCount % 2 == 0 )
                    // {
                        /* Send reported property every other cycle */
                        ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                          sampleazureiotPROPERTY, lPublishCount / 2 + 1 );
                        xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient,
                                                                            ucScratchBuffer, ulScratchBufferLength,
                                                                            NULL );
                        configASSERT( xResult == eAzureIoTSuccess );
                    // }

                    /* Leave Connection Idle for some time. */
                    LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
                    vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
                }

                if( xAzureSample_IsConnectedToInternet() )
                {
                    xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
                    configASSERT( xResult == eAzureIoTSuccess );

                    xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
                    configASSERT( xResult == eAzureIoTSuccess );

                    xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
                    configASSERT( xResult == eAzureIoTSuccess );

                    /* Send an MQTT Disconnect packet over the already connected TLS over
                     * TCP connection. There is no corresponding response for the disconnect
                     * packet. After sending disconnect, client must close the network
                     * connection. */
                    xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
                    configASSERT( xResult == eAzureIoTSuccess );
                }

                /* Close the network connection.  */
                TLS_Socket_Disconnect( &xNetworkContext );

                /* Wait for some time between two iterations to ensure that we do not
                 * bombard the IoT Hub. */
                LogInfo( ( "Demo completed successfully.\r\n" ) );
            }

            LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
        }
    }
    else
    {
        UART_PRINT("Failed to Initialize AzureIoT ! Err: %d \r\n", xResult);
    }
}


void *main_entry(void *args)
{
    uint32_t ticksToSleep;
    int32_t             RetVal = -1;
    HWREG(ICACHE_BASE + 0x84) |= 0x00000001  ;//OSPREY_MX-38
    HWREG(ICACHE_BASE + 0x4) |= 0xc0000000  ;//OSPREY_MX-38
    //HWREG(ICACHE_BASE + 0x4) |= 0x80000000  ;//OSPREY_MX-38, this is for 64M cache, instead CRAM

    ConnectCmd_t ConnectParams;
    OsiReturnVal_e rc;
    char accStr[100];
    uint8_t netIdx;
    WlanNetworkEntry_t   netEntry;

    uint32_t epochTime;

    Board_init();

    GPIO_write(CONFIG_LED_BLUE, 0);

     // init the terminal
     InitTerm();

    /* Open LED0 (green) and LED1 (red) with default params */
    /* NOTE: by default the LEDS are used to reflect the status of WI-FI (green) and
     *       MQTT (red) connections. Once MQTT is connected -  the app will take control
     *       (through MQTT subscriptions).
     *       To control the leds from the APP only - uncomment WIFI_LED_HANDLE (in wifi_settings.h)
     *       and/or MQTT_LED_HANDLE (in mqtt_settings.h)
     */
   LED_IF_init();

    /* Output device information to the UART terminal */
    RetVal = DisplayAppBanner(APPLICATION_NAME, APPLICATION_VERSION);

    if(RetVal < 0)
    {
        /* Handle Error */
        UART_PRINT(
            "Network Terminal - Unable to retrieve device information \n");
        return(NULL);
    }

    rc = WIFI_IF_init(true);
    if (rc == OSI_OK)
    {
        UART_PRINT("WiFi IF init completed successfully.\r\n");
    }
    else
    {
        UART_PRINT("Failed to init WiFi IF Err:%d\r\n", rc);
    }

    // delay needed for proper printout
    osi_Sleep(1);

    rc = WIFI_IF_start(OnWifiEvent, WIFI_SERVICE_LVL_IP, 10000, &hWifiConn);
    if (rc == OSI_OK)
    {
        UART_PRINT("WiFi IF start completed successfully.\r\n");
    }
    else
    {
        UART_PRINT("Failed to start WiFi IF Err:%d\r\n", rc);
    }

    // for(;;);
    /* if predefined AP credentials fail, open interactive mode */
   if (rc != OSI_OK)
   {
       while (1)
       {
           WIFI_IF_scan(WLAN_ROLE_STA);
           osi_Sleep(2);
           RetVal = GetCmd((char *)accStr, 100, "please choose a network index to connect to or any other key to rescan: ");
           netIdx = atoi(accStr);
           if ((netIdx >= 1) && (netIdx <= WIFI_IF_getNetEntrySize()))
           {
               break;
           }
       }

       RetVal = WIFI_IF_getNetEntry(netIdx, &netEntry);
       if (OSI_OK == RetVal)
       {
           if (netEntry.SecurityInfo != 0)
           {
               RetVal = GetCmd((char *)accStr, 100, "please enter the password: ");
               if (strlen((char *)accStr) <= PASSWD_LEN_MAX)
               {
                   UART_PRINT("\n\rconnecting to %s\n\r", netEntry.Ssid);
               }

               rc = WIFI_IF_connect(hWifiConn, netIdx, (int8_t *)accStr, strlen((char *)accStr), WIFI_SERVICE_LVL_IP, 10000);
           }
           else
           {
               UART_PRINT("\n\rconnecting to %s\n\r", netEntry.Ssid);
               rc = WIFI_IF_connect(hWifiConn, netIdx, NULL, 0, WIFI_SERVICE_LVL_IP, 10000);
           }
       }
   }

    while(gIp4Addr == 0)
    {
        osi_uSleep(1000);
    }

    datetime_init();
    sntpWrapper_store_servers(3, SNTP_SERVER_1,SNTP_SERVER_2,SNTP_SERVER_3);
    RetVal = sntpWrapper_updateDateTime();
    if (rc != OSI_OK)
    {
        UART_PRINT("Setting system date/time failed with error %d\n\r", RetVal);
    }

    /* Connect to Azure IoT Hub using AzureIoT APIs */
    prvAzureDemoTask(NULL);

}

void *mainThread(void *args)
{
    main_entry(NULL);
    return NULL;
}

void nmi_handler(void) {
    while (1);
}

void hf_handler(void) {
    while (1);
}

void mpu_handler(void) {
    while (1);
}

void bus_handler(void) {
    while (1);
}

void usage_handler(void) {
    while (1);
}

void secure_handler(void) {
    while (1);
}

void sv_handler(void) {
    while (1);
}

void debug_handler(void) {
    while (1);
}
