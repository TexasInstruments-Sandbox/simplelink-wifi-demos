/**
 * @file aws_iot_ota.h
 * @brief AWS IoT OTA firmware update via IoT Jobs + PSA FWU.
 *
 * Receives OTA job notifications over the shared MQTT connection (from
 * @c aws_iot_telemetry.c), parses the job document, downloads each firmware
 * component from S3 via HTTPS, and installs them using the PSA Firmware
 * Update API.
 *
 * @par Typical call sequence (after telemetry connect)
 * @code
 * AwsIotOta_Init(AwsIotTelemetry_GetMqttCtx());
 * AwsIotOta_HandleTrialState();     // accept or reject TRIAL firmware
 * AwsIotOta_Subscribe();            // subscribe to Jobs topics
 * AwsIotTelemetry_RegisterPublishCallback(AwsIotOta_OnMqttPublish);
 * AwsIotOta_CheckForUpdate();       // poll for any pending jobs
 *
 * // Telemetry run loop returns AWS_IOT_TELEMETRY_OTA_PENDING
 * if (AwsIotOta_IsUpdatePending()) {
 *     AwsIotOta_ExecuteUpdate();    // blocking; reboots on success
 * }
 * @endcode
 */

#ifndef AWS_IOT_OTA_H
#define AWS_IOT_OTA_H

#include <stdint.h>
#include <stdbool.h>

#include "core_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------*/

/** @brief Maximum number of firmware components in a single OTA job. */
#define OTA_MAX_COMPONENTS  3U

/* --------------------------------------------------------------------------
 * Types
 * --------------------------------------------------------------------------*/

/**
 * @brief Firmware component descriptor parsed from an IoT Jobs document.
 *
 * Each field is null-terminated.  The @c url field holds the full presigned
 * S3 URL for the corresponding firmware binary.
 */
typedef struct
{
    char     type[16];    /**< Component type: "BL2", "Wireless_FW", or "Vendor_Image". */
    uint8_t  slot1_id;    /**< Primary slot component ID (0, 2, or 4). */
    uint8_t  slot2_id;    /**< Secondary slot component ID (1, 3, or 5). */
    char     version[16]; /**< Version string "MAJOR.MINOR.PATCH.BUILD". */
    char     url[512];    /**< Full S3 presigned HTTPS URL. */
    uint32_t size;        /**< Total download size in bytes (informational). */
} ota_component_t;

/* --------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/

/**
 * @brief Initialise the OTA module with the shared MQTT context.
 *
 * Stores the MQTT context pointer and caches the thing name.  Must be called
 * before any other function in this module.
 *
 * @param[in] pMqttCtx  Pointer to the active @c MQTTContext_t shared with the
 *                      telemetry module.  Must not be @c NULL.
 *
 * @return @c 0 on success.
 * @return @c -1 if @c pMqttCtx is NULL or the thing name is unavailable.
 */
int AwsIotOta_Init(MQTTContext_t *pMqttCtx);

/**
 * @brief Subscribe to AWS IoT Jobs notification topics.
 *
 * Subscribes to:
 * - @c $aws/things/{name}/jobs/notify-next  (push notification for new jobs)
 * - @c $aws/things/{name}/jobs/$next/get/accepted  (response to poll requests)
 *
 * @pre @c AwsIotOta_Init() must have completed successfully.
 *
 * @return @c 0 on success.
 * @return Negative value on MQTT subscribe failure.
 */
int AwsIotOta_Subscribe(void);

/**
 * @brief Publish a poll request to fetch any pending Jobs immediately.
 *
 * Publishes @c {} to @c $aws/things/{name}/jobs/$next/get.  If a pending job
 * exists, AWS IoT Core responds on the @c $next/get/accepted topic which is
 * already subscribed to by @c AwsIotOta_Subscribe().
 *
 * @pre @c AwsIotOta_Subscribe() must have completed successfully.
 *
 * @return @c 0 on success.
 * @return Negative value on MQTT publish failure.
 */
int AwsIotOta_CheckForUpdate(void);

/**
 * @brief Dispatch an incoming MQTT publish to the OTA handler.
 *
 * Checks whether the topic belongs to the OTA Jobs subscription.  If so,
 * parses the job document and sets the update-pending flag.
 *
 * This function is registered as the telemetry publish callback via
 * @c AwsIotTelemetry_RegisterPublishCallback(AwsIotOta_OnMqttPublish).
 *
 * @param[in] pPublish  Incoming publish information from coreMQTT.
 */
void AwsIotOta_OnMqttPublish(MQTTPublishInfo_t *pPublish);

/**
 * @brief Return @c true if a parsed OTA job is waiting to be executed.
 *
 * Set by @c AwsIotOta_OnMqttPublish() when a valid job document is received.
 * Cleared by @c AwsIotOta_ExecuteUpdate().
 *
 * @return @c true  if @c AwsIotOta_ExecuteUpdate() should be called.
 * @return @c false otherwise.
 */
bool AwsIotOta_IsUpdatePending(void);

/**
 * @brief Execute the pending OTA job (blocking).
 *
 * For each component in the pending job:
 * -# Selects the non-primary target slot.
 * -# Prepares the slot (cancels/cleans stale state).
 * -# Downloads the firmware binary from S3 via HTTPS.
 * -# Streams data to the PSA FWU write API (manifest first, then image).
 * -# Marks the component as CANDIDATE.
 *
 * After all components succeed:
 * -# Calls @c psa_fwu_install() to stage all CANDIDATEs.
 * -# Publishes job status SUCCEEDED to AWS IoT.
 * -# Calls @c psa_fwu_request_reboot() — this function does not return.
 *
 * On any download or FWU error:
 * -# Cancels the current component.
 * -# Publishes job status FAILED to AWS IoT.
 * -# Returns a negative error code (device remains on current firmware).
 *
 * @pre @c AwsIotOta_IsUpdatePending() must return @c true.
 *
 * @return Never returns on success (device reboots).
 * @return Negative value on failure; device stays on current firmware.
 */
int AwsIotOta_ExecuteUpdate(void);

/**
 * @brief Check for TRIAL firmware on boot and accept or reject it.
 *
 * Calls @c psa_fwu_init(), scans all components for TRIAL state, and
 * accepts them if found (indicating the new firmware booted successfully).
 *
 * Call this once early in the post-connect flow, before subscribing to Jobs,
 * so that each new firmware version is confirmed within one boot cycle.
 */
void AwsIotOta_HandleTrialState(void);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_OTA_H */
