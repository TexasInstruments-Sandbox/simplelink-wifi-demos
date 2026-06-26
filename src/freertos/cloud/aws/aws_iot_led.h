/**
 * @file aws_iot_led.h
 * @brief Remote LED control via AWS IoT Classic Device Shadow.
 *
 * Subscribes to the thing's shadow delta and get/accepted topics so that a
 * remote application can turn the on-board LED on or off by updating the
 * shadow's desired state.
 *
 * Shadow document format:
 * @code
 * {
 *   "state": {
 *     "desired":  { "led": "on" },
 *     "reported": { "led": "on" }
 *   }
 * }
 * @endcode
 *
 * @par Usage
 * After @c AwsIotTelemetry_Connect() succeeds, call in order:
 *  1. @c AwsIotLed_Init(AwsIotTelemetry_GetMqttCtx())
 *  2. @c AwsIotLed_Subscribe()
 *  3. @c AwsIotLed_RequestCurrentState()
 *  4. Register @c AwsIotLed_OnMqttPublish via a dispatcher in the
 *     @c AwsIotTelemetry_RegisterPublishCallback call.
 */

#ifndef AWS_IOT_LED_H
#define AWS_IOT_LED_H

#include "core_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the LED shadow module.
 *
 * Caches the MQTT context and pre-builds the shadow topic strings for this
 * device.  Must be called before any other @c AwsIotLed_* function.
 *
 * @param[in] pMqttCtx  Shared MQTT context (from AwsIotTelemetry_GetMqttCtx()).
 * @return @c 0 on success, @c -1 if @p pMqttCtx is NULL or the thing name is
 *         unavailable.
 */
int AwsIotLed_Init(MQTTContext_t *pMqttCtx);

/**
 * @brief Subscribe to shadow delta and get/accepted topics.
 *
 * Subscribes to:
 *  - @c $aws/things/{name}/shadow/update/delta
 *  - @c $aws/things/{name}/shadow/get/accepted
 *
 * @return @c 0 on success, @c -1 on MQTT error.
 */
int AwsIotLed_Subscribe(void);

/**
 * @brief Request the current shadow state from AWS.
 *
 * Publishes @c {} to @c $aws/things/{name}/shadow/get.  AWS responds on the
 * @c get/accepted topic with the full shadow document; the LED is set to
 * @c desired.led if present.
 *
 * Call once after @c AwsIotLed_Subscribe() to restore LED state on reconnect.
 */
void AwsIotLed_RequestCurrentState(void);

/**
 * @brief Route an incoming MQTT PUBLISH to the LED shadow handler.
 *
 * Filters for shadow delta and get/accepted topics.  Non-matching topics are
 * ignored immediately so this can be called from a shared dispatcher.
 *
 * @param[in] pPublish  Incoming publish info from coreMQTT event callback.
 */
void AwsIotLed_OnMqttPublish(MQTTPublishInfo_t *pPublish);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_LED_H */
