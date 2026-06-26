/**
 * @file aws_iot_telemetry.h
 * @brief Periodic telemetry publisher — connects to AWS IoT Core using
 *        device credentials stored in NVOCMP and publishes a JSON payload
 *        to the topic @c "infiniTI/<thingName>/telemetry" at a compile-time
 *        configurable interval.
 *
 * @par Prerequisites
 * - @c AwsIotProv_Init() and a successful provisioning run (or a device that
 *   was provisioned on a prior boot) so that the device certificate, private
 *   key, and thing name are present in NVOCMP.
 * - WiFi is connected and an IPv4 address has been obtained.
 * - @c initialize_mbedtls_threading() has been called.
 *
 * @par Typical call sequence
 * @code
 * // After provisioning block in ble_wifi_provisioning.c:
 * AwsIotTelemetryStatus_t s = AwsIotTelemetry_Connect();
 * if (s == AWS_IOT_TELEMETRY_SUCCESS) {
 *     AwsIotTelemetry_Run();   // blocking; returns only on MQTT error
 * }
 * AwsIotTelemetry_Disconnect();
 * @endcode
 */

#ifndef AWS_IOT_TELEMETRY_H
#define AWS_IOT_TELEMETRY_H

#include "core_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes returned by the telemetry API.
 */
typedef enum
{
    AWS_IOT_TELEMETRY_SUCCESS = 0,           /**< Operation succeeded. */
    AWS_IOT_TELEMETRY_ERROR_INIT,            /**< Could not init I2C and sensors. */
    AWS_IOT_TELEMETRY_ERROR_CREDENTIALS,     /**< Could not read cert/key/name from NVOCMP. */
    AWS_IOT_TELEMETRY_ERROR_CONNECT,         /**< TLS or MQTT connection failed. */
    AWS_IOT_TELEMETRY_ERROR_PUBLISH,         /**< MQTT_Publish returned an error. */
    AWS_IOT_TELEMETRY_OTA_PENDING,           /**< OTA job received; caller should run AwsIotOta_ExecuteUpdate(). */
    AWS_IOT_TELEMETRY_STOPPED,               /**< Tick callback requested an early stop. */
} AwsIotTelemetryStatus_t;

/**
 * @brief Interval between telemetry publishes in milliseconds.
 *
 * MQTT_ProcessLoop is called every second during the wait, so keepalive
 * PINGRESPs are serviced regardless of this value.
 */
#define AWS_IOT_TELEMETRY_PERIOD_MS       (10000U)

/**
 * @brief Maximum byte length of the telemetry JSON payload (including null terminator).
 */
#define AWS_IOT_TELEMETRY_JSON_MAX_LEN    (512U)

/**
 * @defgroup AwsIotTelemetryConfig AWS IoT Telemetry Configuration
 * @brief Compile-time configuration for periodic telemetry publishing.
 * @{
 */

/**
 * @brief MQTT topic prefix for telemetry publishes.
 *
 * Full topic: @c "<AWS_IOT_TELEMETRY_TOPIC_PREFIX>/<thingName>/telemetry"
 * e.g. "AwsTI/DevMac_AABBCCDDEEFF/telemetry"
 */
#define AWS_IOT_TELEMETRY_TOPIC_PREFIX    "AwsTI"

/**
 * @brief Initializes telemetry sensors and I2C interface.
 *
 * @return @c AWS_IOT_TELEMETRY_SUCCESS on success.
 * @return @c AWS_IOT_TELEMETRY_ERROR_INIT if init fails.
 */
AwsIotTelemetryStatus_t AwsIotTelemetry_Init(void);

/**
 * @brief Establish a mutual-TLS MQTT connection to AWS IoT Core using the
 *        device certificate and private key stored in NVOCMP.
 *
 * Reads the device certificate, private key, and thing name from NVOCMP,
 * opens a TLS connection to @c AWS_IOT_ENDPOINT:AWS_IOT_MQTT_PORT, and
 * performs an MQTT CONNECT with the thing name as the client identifier.
 *
 * @return @c AWS_IOT_TELEMETRY_SUCCESS on success.
 * @return @c AWS_IOT_TELEMETRY_ERROR_CONNECT if TLS or MQTT connect fails.
 */
AwsIotTelemetryStatus_t AwsIotTelemetry_Connect(void);

/**
 * @brief Run the telemetry publish loop (blocking).
 *
 * Publishes an example JSON telemetry payload to
 * @c "AwsTI/<thingName>/telemetry" at QoS 1 every
 * @c AWS_IOT_TELEMETRY_PERIOD_MS milliseconds.  Between publishes,
 * @c MQTT_ProcessLoop is called every 1 second to service keepalive
 * PINGRESPs.
 *
 * This function does not return under normal operation.  It returns
 * @c AWS_IOT_TELEMETRY_ERROR_PUBLISH if @c MQTT_Publish fails, allowing
 * the caller to attempt a reconnect or simply log the error.
 *
 * @pre @c AwsIotTelemetry_Connect() must have completed successfully.
 *
 * @return @c AWS_IOT_TELEMETRY_ERROR_PUBLISH on MQTT publish failure.
 */
AwsIotTelemetryStatus_t AwsIotTelemetry_Run(void);

/**
 * @brief Tear down the MQTT session and TLS connection.
 *
 * Calls @c MQTT_Disconnect() followed by @c TLS_Socket_Disconnect().
 * Safe to call even if @c AwsIotTelemetry_Connect() did not fully succeed.
 */
void AwsIotTelemetry_Disconnect(void);

/**
 * @brief Return a pointer to the internal MQTT context.
 *
 * Allows other modules (e.g. the OTA module) to share the same MQTT
 * connection for subscribing to and publishing on additional topics.
 *
 * Valid only after a successful @c AwsIotTelemetry_Connect() call.
 *
 * @return Pointer to the module-level @c MQTTContext_t.
 */
MQTTContext_t *AwsIotTelemetry_GetMqttCtx(void);

/**
 * @brief Register a callback to receive incoming MQTT PUBLISH packets.
 *
 * The registered function is called from within @c MQTT_ProcessLoop()
 * whenever the broker delivers a PUBLISH on any topic the connection has
 * subscribed to.  Used by the OTA module to receive job notifications.
 *
 * Pass @c NULL to deregister.
 *
 * @param[in] cb  Callback with signature @c void cb(MQTTPublishInfo_t *).
 */
void AwsIotTelemetry_RegisterPublishCallback(void (*cb)(MQTTPublishInfo_t *));

/**
 * @brief Register a periodic tick callback invoked every ~100 ms inside
 *        @c AwsIotTelemetry_Run().
 *
 * Use this to perform short, non-blocking work (e.g. button polling) without
 * needing to break the telemetry loop from the outside.  Return non-zero from
 * the callback to request an early stop; @c AwsIotTelemetry_Run() will then
 * return @c AWS_IOT_TELEMETRY_STOPPED.
 *
 * Pass @c NULL to deregister.
 *
 * @param[in] cb  Callback with signature @c int cb(void).
 */
void AwsIotTelemetry_RegisterTickCallback(int (*cb)(void));

/**
 * @brief Retrieve the provisioned AWS IoT Thing name.
 *
 * With current implementation, the name is statically in the code.
 * Next, it should be stored in the flash.
 *
 * @pre 
 *
 * @return Pointer to a null-terminated Thing name string (e.g.
 *         @c "DevMac_AABBCCDDEEFF").  The pointer is valid for the lifetime
 *         of the module (static storage).
 * @return
 */
const char *AwsIot_GetThingName(void);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_TELEMETRY_H */
