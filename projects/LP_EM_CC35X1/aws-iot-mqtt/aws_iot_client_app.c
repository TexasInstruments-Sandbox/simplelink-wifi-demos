#include <aws_iot_client_app.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include <event_groups.h>

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
#include "dns_if.h"
#include "button_if.h"
#include "osi_kernel.h"
#include "uart_term.h"
#include "time.h"
#include "sntp_wrapper.h"
#include "date_time_service.h"


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

/* AWS IoT Hub */
#include "aws_iot_mqtt.h"
#include "aws_iot_telemetry.h"
#include "aws_iot_ota.h"
#include "aws_iot_led.h"

//ERRORS
#include "errors.h"

#include "transport_tls_socket.h"
#include "transport_abstraction.h"

#include <ti/drivers/GPIO.h>

#define SNTP_SERVER_1   "216.239.35.0"
#define SNTP_SERVER_2   "216.239.35.4"
#define SNTP_SERVER_3   "129.6.15.28"

#define BSSID_ADDR              (6)

typedef struct ConnectCmd
{
    uint8_t                 bssid[BSSID_ADDR];
    /* Ap's SSID */
    uint8_t                 *ssid;
    /* Security parameters - Security Type and Password */
    WlanSecParams_t secParams;
}ConnectCmd_t;

static ip4addr_t gIp4Addr = 0, gIp4Mask = 0, gIp4GW = 0;
void *hWifiConn;
static bool s_is_connected_to_internet = false;
static EventGroupHandle_t  s_btnEvents;
static StaticEventGroup_t  s_egBuf;

/*-----------------------------------------------------------*/

/** @brief Bitmask of pending button events. */
#define BTN_EVT_SW1  ((EventBits_t)(1u << 0))  /**< SW1: connect/disconnect from AWS broker */
#define BTN_EVT_SW2  ((EventBits_t)(1u << 1))  /**< SW2: trigger an OTA */

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleawsiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 10000U ) )


/**
 * @brief The function used to demonstrate the connection to AWS IOT via MQTT.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAwsDemoTask( void * pvParameters );

EventBits_t ButtonHandler_Poll(void)
{
    return xEventGroupGetBits(s_btnEvents);
}

void ButtonHandler_ClearEvents(EventBits_t mask)
{
    xEventGroupClearBits(s_btnEvents, mask);
}

void ButtonSw1EventHandler(BUTTON_IF_events_bm events)
{
    BaseType_t higher = pdFALSE;

    if ( (events & BUTTON_IF_EV_CLICKED) || (events & BUTTON_IF_EV_LONG_CLICKED) )
    {
        LogInfo( ("SW1 button pressed, trigger AWS connect/disconnect\r\n") );

        xEventGroupSetBitsFromISR(s_btnEvents, BTN_EVT_SW1, &higher);
        // Yield ONLY if higher-priority task is waiting
        if (higher == pdTRUE)
        {
            portYIELD_FROM_ISR(higher);
        }
    }
}

void ButtonSw2EventHandler(BUTTON_IF_events_bm events)
{
    BaseType_t higher = pdFALSE;

    if ( (events & BUTTON_IF_EV_CLICKED) || (events & BUTTON_IF_EV_LONG_CLICKED) )
    {
        LogInfo( ("SW1 button pressed, trigger OTA update\r\n") );

        xEventGroupSetBitsFromISR(s_btnEvents, BTN_EVT_SW2, &higher);
        // Yield ONLY if higher-priority task is waiting
        if (higher == pdTRUE)
        {
            portYIELD_FROM_ISR(higher);
        }
    }
}

/**
 * @brief Tick callback invoked every ~100 ms by AwsIotTelemetry_Run().
 *
 * Polls SW1 and SW2 so that button presses take effect while the device
 * is actively connected and the telemetry loop is blocking.
 *
 * @return 1 if the telemetry loop should stop (SW1 pressed), 0 otherwise.
 */
static int s_telemetry_tick(void)
{
    /* SW1: connect/disconnect to/from AWS broker */
    if (ButtonHandler_Poll() & BTN_EVT_SW1)
    {
        ButtonHandler_ClearEvents(BTN_EVT_SW1);
        LogInfo( ("trigger AWS connect/disconnect\r\n") );
        return 1;
    }

    /* SW2: trigger an OTA update */
    if (ButtonHandler_Poll() & BTN_EVT_SW2)
    {
        ButtonHandler_ClearEvents(BTN_EVT_SW2);
        LogInfo( ("trigger OTA update\r\n") );
    }

    return 0;
}                                        

bool xAwsSample_IsConnectedToInternet()
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


/**
 * @brief Shared MQTT PUBLISH dispatcher.
 *
 * Called by the coreMQTT event callback for every incoming PUBLISH.
 * Routes the message to each module that may be interested; modules that
 * don't recognise the topic return immediately.
 */
static void s_mqtt_dispatch(MQTTPublishInfo_t *pPublish)
{
    AwsIotOta_OnMqttPublish(pPublish);
    AwsIotLed_OnMqttPublish(pPublish);
}


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
    switch (status)
    {
        case WIFI_STATUS_CONNECTED_IP:
        {
            TCPIP_IF_getIp4Addr(params, &gIp4Addr, &gIp4Mask, &gIp4GW);
            if(gIp4Addr)
            {
                LogInfo( ("OnWifiEvent(CONNECTED_IP): addr=%s\r\n", IP4ToStr(gIp4Addr)) );
                LogInfo( ("                            mask=%s\r\n", IP4ToStr(gIp4Mask)) );
                LogInfo( ("                            gw=%s\r\n", IP4ToStr(gIp4GW)) );
            }
            else
            {
                LogInfo( ("OnWifiEvent(CONNECTED_IP): IPv6 only\r\n") );
            }
            s_is_connected_to_internet = true;
        }
        break;

        case WIFI_STATUS_DISCONNECTED:
            LogInfo( ("OnWifiEvent - Disconnected (%d)\r\n", status) );
            s_is_connected_to_internet = false;
            break;
        default:
            LogInfo( ("OnWifiEvent(%d)\r\n", status) );
            break;
    }
}


/**
 * @brief AWS IoT demo task that gets started in the platform specific project.
 */
void prvAwsDemoTask( void * pvParameters )
{
    AwsIotTelemetryStatus_t telStatus;

    ( void ) pvParameters;

    LogInfo( ("[Main] AWS IoT demo - publish and subscribe.\r\n") );

    telStatus = AwsIotTelemetry_Init();
    if (telStatus == AWS_IOT_TELEMETRY_SUCCESS)
    {
        LogInfo( ("Sensors initialized.\r\n") );
    }
    else
    {
        LogError( ("Failed to initialize sensors.\r\n") );
    }

    for( ; ; )
    {
        if( xAwsSample_IsConnectedToInternet() )
        {
            LogInfo( ("Connecting to AWS IoT Core...\r\n") );
            
            telStatus = AwsIotTelemetry_Connect();
            if (telStatus == AWS_IOT_TELEMETRY_SUCCESS)
            {
                LogInfo( ("Connected. Publishing every %u ms.\r\n",
                        (unsigned)AWS_IOT_TELEMETRY_PERIOD_MS) );

                /* Route all incoming PUBLISHes through the shared dispatcher */
                AwsIotTelemetry_RegisterPublishCallback(s_mqtt_dispatch);

		        /* OTA initialisation — shares the telemetry MQTT connection */
                if (AwsIotOta_Init(AwsIotTelemetry_GetMqttCtx()) == 0)
	            {
                    AwsIotOta_HandleTrialState();   /* accept/reject TRIAL firmware */
                    AwsIotOta_Subscribe();          /* subscribe to Jobs notification topics */
                    AwsIotOta_CheckForUpdate();     /* poll for any job pending before boot */
	            }

                /* LED shadow control â€” shares the telemetry MQTT connection */
                if (AwsIotLed_Init(AwsIotTelemetry_GetMqttCtx()) == 0)
                {
                    AwsIotLed_Subscribe();
                    AwsIotLed_RequestCurrentState();
                }

                /* Register the tick callback so SW1/SW2 are serviced every ~100 ms
                * even while the telemetry loop is blocking. */
                AwsIotTelemetry_RegisterTickCallback(s_telemetry_tick);

	            /* Run telemetry and service OTA jobs.  Loop so that a no-reboot
                * OTA result (version already installed) resumes telemetry rather
                * than ending the session.  Real OTA success reboots inside
                * ExecuteUpdate and never returns here.  Any failure or non-OTA
                * telemetry exit breaks out and disconnects. */
	            while (1)
	            {
	                AwsIotTelemetryStatus_t runStatus = AwsIotTelemetry_Run();

	                if (runStatus != AWS_IOT_TELEMETRY_OTA_PENDING)
	                {
	                    break;
	                }				
	                /* ExecuteUpdate returned 0 without rebooting (target version
	                 * already installed) — loop back to resume telemetry. */
	            }				
            }
        	else
        	{
            		LogError( ("Connect failed (error %d).\r\n",
                     	  (int)telStatus) );
        	}
        	AwsIotTelemetry_Disconnect();

        }

        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( sampleawsiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}


void *main_entry(void *args)
{
    int32_t RetVal = -1;

    HWREG(ICACHE_BASE + 0x84) |= 0x00000001  ;//OSPREY_MX-38
    HWREG(ICACHE_BASE + 0x4) |= 0xc0000000  ;//OSPREY_MX-38
    //HWREG(ICACHE_BASE + 0x4) |= 0x80000000  ;//OSPREY_MX-38, this is for 64M cache, instead CRAM

    OsiReturnVal_e rc;
    char accStr[100];
    uint8_t netIdx;
    WlanNetworkEntry_t   netEntry;

    Board_init();

    BUTTON_IF_init();
    BUTTON_IF_registertCallback(CONFIG_BUTTON_SW1, CONFIG_GPIO_BUTTON_SW1_INPUT,
                                ButtonSw1EventHandler, BUTTON_IF_EV_CLICKED, 1000U);
    BUTTON_IF_registertCallback(CONFIG_BUTTON_SW2, CONFIG_GPIO_BUTTON_SW2_INPUT,
                                ButtonSw2EventHandler, BUTTON_IF_EV_CLICKED, 1000U);

    BUTTON_IF_enable(CONFIG_BUTTON_SW1);
    BUTTON_IF_enable(CONFIG_BUTTON_SW2);

    // init the terminal
    InitTerm();

    /* Open all LEDs */
    /* NOTE: by default, the green LED is used to reflect the status of WI-FI.
     *       Once Wi-Fi is connected, the app will take control over it.
     *       To control the leds from the APP only - uncomment WIFI_LED_HANDLE (in wifi_settings.h)
     */
    LED_IF_init();

    /* initializes the event group */
    s_btnEvents = xEventGroupCreateStatic(&s_egBuf);
    /* clear all requests on switches */
    ButtonHandler_ClearEvents(BTN_EVT_SW1 | BTN_EVT_SW2);

    /* Output device information to the UART terminal */
    RetVal = DisplayAppBanner(APPLICATION_NAME, APPLICATION_VERSION);

    if(RetVal < 0)
    {
        /* Handle Error */
        LogError( (
            "Network Terminal - Unable to retrieve device information \n");
        return(NULL) );
    }

    rc = WIFI_IF_init(true);
    if (rc == OSI_OK)
    {
        LogInfo( ("WiFi IF init completed successfully.\r\n") );
    }
    else
    {
        LogError( ("Failed to init WiFi IF Err:%d\r\n", rc) );
    }

    // delay needed for proper printout
    osi_Sleep(1);

    rc = WIFI_IF_start(OnWifiEvent, WIFI_SERVICE_LVL_IP, 10000, &hWifiConn);
    if (rc == OSI_OK)
    {
        LogInfo( ("WiFi IF start completed successfully.\r\n") );
    }
    else
    {
        LogError( ("Failed to start WiFi IF Err:%d\r\n", rc) );
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
                   LogInfo( ("connecting to %s\n\r", netEntry.Ssid) );
               }

               rc = WIFI_IF_connect(hWifiConn, netIdx, (int8_t *)accStr, strlen((char *)accStr), WIFI_SERVICE_LVL_IP, 10000);
           }
           else
           {
               LogInfo( ("connecting to %s\n\r", netEntry.Ssid) );
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
        LogError( ("Setting system date/time failed with error %d\n\r", RetVal) );
    }

    /* Connect to Azure IoT Hub using AzureIoT APIs */
    prvAwsDemoTask(NULL);

    return NULL;
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
