/**
 * @file aws_iot_led.c
 * @brief Remote LED control via AWS IoT Classic Device Shadow.
 *
 * Receives shadow delta notifications and applies the desired LED state using
 * the TI Drivers GPIO API.  After applying a command the module publishes the
 * reported state back so the shadow document stays in sync.
 *
 * @par MQTT packet IDs
 * Uses 100–199 to avoid collision with telemetry (starts at 1) and OTA (200+).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Board Header files */
#include "ti_drivers_config.h"

/* Utilities */
#include "led_if.h"

/* AWS IoT service headers */
#include "aws_iot_led.h"
#include "aws_iot_telemetry.h"       /* AwsIot_GetThingName() */
#include "demo_config.h"

/* coreMQTT */
#include "core_mqtt.h"

/* coreJSON */
#include "core_json.h"

/* UART terminal */
#include "uart_term.h"

/* --------------------------------------------------------------------------
 * Compile-time configuration
 * --------------------------------------------------------------------------*/

/** MQTT packet ID base for LED shadow operations. */
#define LED_MQTT_PACKET_ID_BASE  100U

/* --------------------------------------------------------------------------
 * Module-level state
 * --------------------------------------------------------------------------*/

/** Shared MQTT context (owned by aws_iot_telemetry.c). */
static MQTTContext_t *s_mqttCtx = NULL;

/** Cached thing name. */
static char s_thingName[AWS_IOT_MAX_THING_NAME];

/** Monotonically incrementing MQTT packet ID for LED operations. */
static uint16_t s_packetId = LED_MQTT_PACKET_ID_BASE;

/** Pre-built shadow topic strings. */
static char s_topicDelta[144];        /**< shadow/update/delta    */
static char s_topicGetAccepted[144];  /**< shadow/get/accepted    */
static char s_topicUpdate[128];       /**< shadow/update          */
static char s_topicGet[128];          /**< shadow/get             */

/* --------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------*/

static uint16_t next_packet_id(void)
{
    uint16_t id = s_packetId++;
    if (s_packetId == 0U || s_packetId >= 200U)
    {
        s_packetId = LED_MQTT_PACKET_ID_BASE;
    }
    return id;
}

/**
 * @brief Apply an LED command received from the shadow.
 *
 * @param[in] pVal    Pointer to the value string (not null-terminated).
 * @param[in] valLen  Length of @p pVal in bytes.
 * @param[in] ledIdx  index to the LED as appears in ti_drivers_config header.
 * @param[out] stateStr  string indicating the LED status.
 */
static char * s_apply_led_command(const char *pVal, size_t valLen, uint8_t ledIdx)
{
    const char *stateStr;

    if (valLen == 2u && strncmp(pVal, "on", 2) == 0)
    {
        LED_IF_set(CONFIG_LED_RED, 100);
        stateStr = "on";
        UART_PRINT("[LED] LED turned ON\r\n");
    }
    else if (valLen == 3u && strncmp(pVal, "off", 3) == 0)
    {
        LED_IF_set(CONFIG_LED_RED, 0);
        stateStr = "off";
        UART_PRINT("[LED] LED turned OFF\r\n");
    }
    else
    {
        UART_PRINT("[LED] Unknown LED command value (len=%u)\r\n",
                   (unsigned)valLen);
        return NULL;
    }

    return stateStr;
}

/* --------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/

int AwsIotLed_Init(MQTTContext_t *pMqttCtx)
{
    const char *thingName;

    if (pMqttCtx == NULL)
    {
        UART_PRINT("[LED] Init failed: NULL MQTT context\r\n");
        return -1;
    }

    thingName = AwsIot_GetThingName();
    if (thingName == NULL || thingName[0] == '\0')
    {
        UART_PRINT("[LED] Init failed: thing name unavailable\r\n");
        return -1;
    }

    /* turn off all LEDs */
    LED_IF_set(CONFIG_LED_RED, 0);
    LED_IF_set(CONFIG_LED_GREEN, 0);
    LED_IF_set(CONFIG_LED_BLUE, 0);

    s_mqttCtx = pMqttCtx;
    snprintf(s_thingName, sizeof(s_thingName), "%s", thingName);

    snprintf(s_topicDelta, sizeof(s_topicDelta),
             "$aws/things/%s/shadow/update/delta", s_thingName);
    snprintf(s_topicGetAccepted, sizeof(s_topicGetAccepted),
             "$aws/things/%s/shadow/get/accepted", s_thingName);
    snprintf(s_topicUpdate, sizeof(s_topicUpdate),
             "$aws/things/%s/shadow/update", s_thingName);
    snprintf(s_topicGet, sizeof(s_topicGet),
             "$aws/things/%s/shadow/get", s_thingName);

    UART_PRINT("[LED] Initialised for thing: %s\r\n", s_thingName);
    return 0;
}

int AwsIotLed_Subscribe(void)
{
    MQTTSubscribeInfo_t subs[2];
    MQTTStatus_t        ret;

    if (s_mqttCtx == NULL)
    {
        UART_PRINT("[LED] Subscribe failed: not initialised\r\n");
        return -1;
    }

    /* shadow/update/delta — pushed by AWS when desired ≠ reported */
    subs[0].qos               = MQTTQoS0;
    subs[0].pTopicFilter      = s_topicDelta;
    subs[0].topicFilterLength = (uint16_t)strlen(s_topicDelta);

    /* shadow/get/accepted — response to our startup state request */
    subs[1].qos               = MQTTQoS0;
    subs[1].pTopicFilter      = s_topicGetAccepted;
    subs[1].topicFilterLength = (uint16_t)strlen(s_topicGetAccepted);

    ret = MQTT_Subscribe(s_mqttCtx, subs, 2u, next_packet_id());
    if (ret != MQTTSuccess)
    {
        UART_PRINT("[LED] MQTT_Subscribe failed: %d\r\n", (int)ret);
        return -1;
    }

    /* Drive the process loop once to receive the SUBACK */
    MQTT_ProcessLoop(s_mqttCtx);

    UART_PRINT("[LED] Subscribed to shadow topics\r\n");
    return 0;
}

void AwsIotLed_RequestCurrentState(void)
{
    MQTTPublishInfo_t pub;

    if (s_mqttCtx == NULL)
    {
        return;
    }

    memset(&pub, 0, sizeof(pub));
    pub.qos             = MQTTQoS0;
    pub.pTopicName      = s_topicGet;
    pub.topicNameLength = (uint16_t)strlen(s_topicGet);
    pub.pPayload        = "{}";
    pub.payloadLength   = 2u;

    MQTT_Publish(s_mqttCtx, &pub, 0U);
    UART_PRINT("[LED] Requested current shadow state\r\n");
}

void AwsIotLed_OnMqttPublish(MQTTPublishInfo_t *pPublish)
{
    const char  *pTopic;
    uint16_t     topicLen;
    bool         isDelta;
    bool         isGetAccepted;
    const char  *pVal   = NULL;
    size_t       valLen = 0;
    JSONStatus_t jret;
    char         ledStates[CONFIG_TI_DRIVERS_LED_COUNT][4];  // 3 LEDs, max 4 chars ("off" + null terminator)
    char         reportedPayload[64];

    if (pPublish == NULL || pPublish->pTopicName == NULL)
    {
        return;
    }

    pTopic   = pPublish->pTopicName;
    topicLen = pPublish->topicNameLength;

    isDelta       = (topicLen == (uint16_t)strlen(s_topicDelta) &&
                     strncmp(pTopic, s_topicDelta, topicLen) == 0);
    isGetAccepted = (topicLen == (uint16_t)strlen(s_topicGetAccepted) &&
                     strncmp(pTopic, s_topicGetAccepted, topicLen) == 0);

    if (!isDelta && !isGetAccepted)
    {
        return; /* Not a shadow topic for this module */
    }

    if (pPublish->pPayload == NULL || pPublish->payloadLength == 0)
    {
        return;
    }

    if (isDelta)
    {
        /* Delta payload: {"version":N,"state":{"green_led":"on/off","blue_led":"on/off","red_led":"on/off"},...}
         * Key path within the JSON document: e.g. "state.green_led" */
        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.green_led", 15u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow delta received: green_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_GREEN], s_apply_led_command(pVal, valLen, CONFIG_LED_GREEN));
        }
        
        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.blue_led", 14u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow delta received: blue_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_BLUE], s_apply_led_command(pVal, valLen, CONFIG_LED_BLUE));
        }

        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.red_led", 13u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow delta received: red_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_RED], s_apply_led_command(pVal, valLen, CONFIG_LED_RED));
        }
    }
    else /* isGetAccepted */
    {
        /* Get/accepted payload: full shadow document.
         * Key path for the desired LED state: e.g. "state.desired.green_led" */
        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.desired.green_led", 23u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow get/accepted: desired green_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_GREEN], s_apply_led_command(pVal, valLen, CONFIG_LED_GREEN));
        }
        else
        {
            UART_PRINT("[LED] Shadow get/accepted: no desired.green_led — LED unchanged\r\n");
        }

        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.desired.blue_led", 22u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow get/accepted: desired blue_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_BLUE], s_apply_led_command(pVal, valLen, CONFIG_LED_BLUE));
        }
        else
        {
            UART_PRINT("[LED] Shadow get/accepted: no desired.blue_led — LED unchanged\r\n");
        }

        jret = JSON_SearchConst((const char *)pPublish->pPayload,
                                pPublish->payloadLength,
                                "state.desired.red_led", 21u,
                                &pVal, &valLen, NULL);
        if (jret == JSONSuccess)
        {
            UART_PRINT("[LED] Shadow get/accepted: desired red_led=%.*s\r\n",
                       (int)valLen, pVal);
            strcpy(ledStates[CONFIG_LED_RED], s_apply_led_command(pVal, valLen, CONFIG_LED_RED));
        }
        else
        {
            UART_PRINT("[LED] Shadow get/accepted: no desired.red_led — LED unchanged\r\n");
        }
    }

    /* Publish reported state so the shadow document stays synchronised */
    snprintf(reportedPayload, sizeof(reportedPayload),
             "{\"state\":{\"reported\":{\"green_led\":\"%s\",\"blue_led\":\"%s\",\"red_led\":\"%s\"}}}", ledStates[CONFIG_LED_GREEN], ledStates[CONFIG_LED_BLUE], ledStates[CONFIG_LED_RED]);

    MQTTPublishInfo_t pub;
    memset(&pub, 0, sizeof(pub));
    pub.qos             = MQTTQoS0;
    pub.pTopicName      = s_topicUpdate;
    pub.topicNameLength = (uint16_t)strlen(s_topicUpdate);
    pub.pPayload        = reportedPayload;
    pub.payloadLength   = strlen(reportedPayload);

    MQTTStatus_t ret = MQTT_Publish(s_mqttCtx, &pub, 0U);
    if (ret != MQTTSuccess)
    {
        UART_PRINT("[LED] Failed to publish reported state: %d\r\n", (int)ret);
    }
}
