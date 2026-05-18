/**
 * @file aws_iot_ota.c
 * @brief AWS IoT OTA firmware update via IoT Jobs + PSA FWU.
 *
 * Implements the full OTA lifecycle:
 * -# Boot-time TRIAL acceptance (@c AwsIotOta_HandleTrialState).
 * -# Job notification via MQTT Jobs topics (@c AwsIotOta_Subscribe,
 *    @c AwsIotOta_CheckForUpdate, @c AwsIotOta_OnMqttPublish).
 * -# Firmware download + FWU write (@c AwsIotOta_ExecuteUpdate).
 *
 * @par FWU helpers
 * Static implementations of @c OTA_FWU_selectTargetSlot(),
 * @c OTA_FWU_prepareSlot(), and @c OTA_FWU_scanPendingStates() are inlined
 * from the TI SDK @c ota_example/ota_fwu.c.
 *
 * @par MQTT sharing
 * The module stores a pointer to the @c MQTTContext_t owned by
 * @c aws_iot_telemetry.c.  Packet IDs for OTA MQTT operations start at 200
 * to avoid collisions with telemetry's in-flight QoS 1 records.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* AWS IoT */
#include "aws_iot_ota.h"
#include "demo_config.h"
#include "aws_iot_ota_https.h"
#include "aws_iot_telemetry.h"       /* AwsIot_GetThingName() */

/* coreMQTT */
#include "core_mqtt.h"

/* coreJSON */
#include "core_json.h"

/* PSA FWU — requires FWU include paths in .cproject */
#include "psa_fwu.h"

/* UART terminal */
#include "uart_term.h"

/* --------------------------------------------------------------------------
 * Compile-time configuration
 * --------------------------------------------------------------------------*/

/** @brief MQTT packet ID base for OTA operations. */
#define OTA_MQTT_PACKET_ID_BASE  200U

/** @brief Maximum byte length of a job ID string (null-terminated). */
#define OTA_MAX_JOB_ID_LEN       64U

/** @brief Minimum of two unsigned values. */
#define OTA_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* --------------------------------------------------------------------------
 * Module-level state
 * --------------------------------------------------------------------------*/

/** @brief Shared MQTT context (owned by aws_iot_telemetry.c). */
static MQTTContext_t *s_mqttCtx = NULL;

/** @brief Cached AWS IoT Thing name. */
static char s_thingName[AWS_IOT_MAX_THING_NAME];

/** @brief Monotonically incrementing MQTT packet ID for OTA operations. */
static uint16_t s_packetId = OTA_MQTT_PACKET_ID_BASE;

/** @brief Pre-built topic: @c $aws/things/NAME/jobs/notify-next */
static char s_topicNotifyNext[128];

/** @brief Pre-built topic: @c $aws/things/NAME/jobs/$next/get */
static char s_topicNextGet[128];

/** @brief Pre-built topic: @c $aws/things/NAME/jobs/$next/get/accepted */
static char s_topicNextGetAccepted[144];

/** @brief Set to @c true when a valid job document is ready to execute. */
static bool s_update_pending = false;

/* --------------------------------------------------------------------------
 * Pending job storage
 * --------------------------------------------------------------------------*/

typedef struct
{
    char           jobId[OTA_MAX_JOB_ID_LEN];
    uint8_t        numComponents;
    ota_component_t components[OTA_MAX_COMPONENTS];
} ota_job_t;

/** @brief Parsed job document from the most recent notification. */
static ota_job_t s_job;

/* --------------------------------------------------------------------------
 * FWU download state (used inside AwsIotOta_ExecuteUpdate)
 * --------------------------------------------------------------------------*/

typedef struct
{
    psa_fwu_component_t targetSlot;  /**< PSA FWU target component ID.       */
    uint32_t            offset;      /**< Total bytes received so far.        */
    int                 started;     /**< Non-zero after psa_fwu_start().     */
    int                 error;       /**< Set non-zero on FWU write failure.  */
    uint8_t             manifest[TI_FWU_MANIFEST_SIZE]; /**< Manifest accumulator. */
} ota_dl_ctx_t;

/* --------------------------------------------------------------------------
 * FWU component name table (from TI ota_fwu.c)
 * --------------------------------------------------------------------------*/

static const char *s_componentName[MAX_COMPONENT_ID] = {
    "BL2_Slot_1",
    "BL2_Slot_2",
    "Wireless_FW_Slot_1",
    "Wireless_FW_Slot_2",
    "Vendor_Image_Slot_1",
    "Vendor_Image_Slot_2",
};

static const char *s_componentState[] = {
    "READY",
    "WRITING",
    "CANDIDATE",
    "STAGED",
    "FAILED",
    "TRIAL",
    "REJECTED",
    "UPDATED",
};

/* --------------------------------------------------------------------------
 * Static FWU helpers (inlined from TI ota_example/ota_fwu.c)
 * --------------------------------------------------------------------------*/

/**
 * @brief Select the non-primary target slot for a component pair.
 *
 * Queries both slots and picks the one that is not the primary (active) slot.
 * Returns the primary slot's version for informational logging.
 *
 * @param[in]  slot1Id          First slot component ID.
 * @param[in]  slot2Id          Second slot component ID.
 * @param[out] pTargetId        Selected target component ID.
 * @param[out] pPrimaryVersion  Version of the primary (active) slot.
 *
 * @return @c 0 on success, @c -1 if no suitable slot found.
 */
static int s_fwu_selectTargetSlot(int slot1Id, int slot2Id,
                                  psa_fwu_component_t *pTargetId,
                                  psa_fwu_image_version_t *pPrimaryVersion)
{
    psa_fwu_component_info_t info1, info2;
    int ret1, ret2;

    ret1 = psa_fwu_query((psa_fwu_component_t)slot1Id, &info1);
    ret2 = psa_fwu_query((psa_fwu_component_t)slot2Id, &info2);

    if (ret1 != PSA_SUCCESS || ret2 != PSA_SUCCESS)
    {
        UART_PRINT("[OTA] Failed to query slots %d/%d (ret1=%d ret2=%d)\r\n",
                   slot1Id, slot2Id, ret1, ret2);
        return -1;
    }

    if (info1.impl.Primary && !info2.impl.Primary)
    {
        if (pPrimaryVersion != NULL) { *pPrimaryVersion = info1.version; }
        *pTargetId = (psa_fwu_component_t)slot2Id;
        return 0;
    }
    else if (info2.impl.Primary && !info1.impl.Primary)
    {
        if (pPrimaryVersion != NULL) { *pPrimaryVersion = info2.version; }
        *pTargetId = (psa_fwu_component_t)slot1Id;
        return 0;
    }

    UART_PRINT("[OTA] No suitable target slot for %d/%d\r\n", slot1Id, slot2Id);
    return -1;
}

/**
 * @brief Prepare a slot for writing by transitioning it to READY state.
 *
 * Handles all source states: cancels WRITING/CANDIDATE, rejects STAGED/TRIAL,
 * and cleans FAILED/REJECTED/UPDATED slots.
 *
 * @param[in] componentId  PSA FWU component ID of the target slot.
 *
 * @return @c 0 if the slot is READY for writing.
 * @return @c -1 on any error.
 */
static int s_fwu_prepareSlot(psa_fwu_component_t componentId)
{
    psa_fwu_component_info_t info;
    int ret;

    ret = psa_fwu_query(componentId, &info);
    if (ret != PSA_SUCCESS)
    {
        UART_PRINT("[OTA] psa_fwu_query(%d) failed: %d\r\n", componentId, ret);
        return -1;
    }

    UART_PRINT("[OTA] Component %d (%s): state=%s, primary=%s, version=%u.%u.%u.%lu\r\n",
               componentId,
               (componentId < MAX_COMPONENT_ID) ? s_componentName[componentId] : "?",
               (info.state <= PSA_FWU_UPDATED) ? s_componentState[info.state] : "?",
               info.impl.Primary ? "yes" : "no",
               info.version.major, info.version.minor,
               info.version.patch, (unsigned long)info.version.build);

    if (info.impl.Primary)
    {
        UART_PRINT("[OTA] Error: component %d is PRIMARY — cannot update\r\n",
                   componentId);
        return -1;
    }

    switch (info.state)
    {
        case PSA_FWU_READY:
            /* Nothing to do */
            break;

        case PSA_FWU_FAILED:
        case PSA_FWU_UPDATED:
            UART_PRINT("[OTA] Cleaning slot %d (state=%s)...\r\n",
                       componentId, s_componentState[info.state]);
            ret = psa_fwu_clean(componentId);
            if (ret != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_clean(%d) failed: %d\r\n", componentId, ret);
                return -1;
            }
            break;

        case PSA_FWU_WRITING:
        case PSA_FWU_CANDIDATE:
            UART_PRINT("[OTA] Cancelling+cleaning slot %d (state=%s)...\r\n",
                       componentId, s_componentState[info.state]);
            ret = psa_fwu_cancel(componentId);
            if (ret != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_cancel(%d) failed: %d\r\n", componentId, ret);
                return -1;
            }
            ret = psa_fwu_clean(componentId);
            if (ret != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_clean(%d) failed: %d\r\n", componentId, ret);
                return -1;
            }
            break;

        case PSA_FWU_STAGED:
        case PSA_FWU_TRIAL:
            UART_PRINT("[OTA] Rejecting+cleaning slot %d (state=%s)...\r\n",
                       componentId, s_componentState[info.state]);
            ret = psa_fwu_reject(PSA_ERROR_NOT_PERMITTED);
            if (ret != PSA_SUCCESS && ret != PSA_SUCCESS_REBOOT)
            {
                UART_PRINT("[OTA] psa_fwu_reject failed: %d\r\n", ret);
                return -1;
            }
            ret = psa_fwu_clean(componentId);
            if (ret != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_clean(%d) after reject failed: %d\r\n",
                           componentId, ret);
                return -1;
            }
            break;

        case PSA_FWU_REJECTED:
            UART_PRINT("[OTA] Cleaning REJECTED slot %d...\r\n", componentId);
            ret = psa_fwu_clean(componentId);
            if (ret != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_clean(%d) failed: %d\r\n", componentId, ret);
                return -1;
            }
            break;

        default:
            UART_PRINT("[OTA] Component %d: unexpected state %u\r\n",
                       componentId, info.state);
            return -1;
    }

    /* Verify the slot is now READY */
    ret = psa_fwu_query(componentId, &info);
    if (ret != PSA_SUCCESS || info.state != PSA_FWU_READY)
    {
        UART_PRINT("[OTA] Component %d not READY after prepare (state=%u)\r\n",
                   componentId, info.state);
        return -1;
    }

    UART_PRINT("[OTA] Slot %d is READY\r\n", componentId);
    return 0;
}

/**
 * @brief Scan all components for TRIAL state (called at startup).
 *
 * @param[out] pTrialCount  Set to the number of TRIAL components found.
 */
static void s_fwu_scanPendingStates(int *pTrialCount)
{
    psa_fwu_component_info_t info;
    int ci, ret;

    if (pTrialCount != NULL) { *pTrialCount = 0; }

    for (ci = 0; ci < MAX_COMPONENT_ID; ci++)
    {
        ret = psa_fwu_query((psa_fwu_component_t)ci, &info);
        if (ret != PSA_SUCCESS) { continue; }

        if (info.state == PSA_FWU_TRIAL)
        {
            UART_PRINT("[OTA] Component %d (%s): %u.%u.%u.%lu [TRIAL]\r\n",
                       ci,
                       (ci < MAX_COMPONENT_ID) ? s_componentName[ci] : "?",
                       info.version.major, info.version.minor,
                       info.version.patch, (unsigned long)info.version.build);
            if (pTrialCount != NULL) { (*pTrialCount)++; }
        }
    }
}

/* --------------------------------------------------------------------------
 * PSA FWU write callback (called by ota_https_download for each body chunk)
 * --------------------------------------------------------------------------*/

/**
 * @brief Stream downloaded bytes into the PSA FWU API.
 *
 * The first @c TI_FWU_MANIFEST_SIZE bytes are buffered into the manifest
 * field of the context and passed to @c psa_fwu_start() when complete.
 * All subsequent bytes are forwarded to @c psa_fwu_write() at the correct
 * image offset (offset within the image = total offset - manifest size).
 *
 * @param[in] pData  Data chunk.
 * @param[in] len    Byte count.
 * @param[in] pCtx   Pointer to an @c ota_dl_ctx_t.
 *
 * @return @c 0 on success, non-zero to abort the download.
 */
static int s_fwu_write_cb(const uint8_t *pData, uint32_t len, void *pCtx)
{
    ota_dl_ctx_t *s = (ota_dl_ctx_t *)pCtx;
    psa_status_t  psaRet;

    if (s == NULL || s->error != 0)
    {
        return -1;
    }

    /* ------------------------------------------------------------------
     * Phase 1: accumulate the manifest header
     * ------------------------------------------------------------------*/
    if (s->offset < (uint32_t)TI_FWU_MANIFEST_SIZE)
    {
        uint32_t toBuffer = OTA_MIN(len,
                                    (uint32_t)TI_FWU_MANIFEST_SIZE - s->offset);
        memcpy(s->manifest + s->offset, pData, toBuffer);
        s->offset += toBuffer;
        pData     += toBuffer;
        len       -= toBuffer;

        if (s->offset == (uint32_t)TI_FWU_MANIFEST_SIZE)
        {
            /* Manifest complete — start the FWU write session */
            psaRet = psa_fwu_start(s->targetSlot, s->manifest,
                                   (size_t)TI_FWU_MANIFEST_SIZE);
            if (psaRet != PSA_SUCCESS)
            {
                UART_PRINT("[OTA] psa_fwu_start(%d) failed: %d\r\n",
                           s->targetSlot, (int)psaRet);
                s->error = (int)psaRet;
                return -1;
            }
            s->started = 1;
            UART_PRINT("[OTA] Manifest received, FWU write started\r\n");
        }
    }

    /* ------------------------------------------------------------------
     * Phase 2: stream image bytes to FWU
     * ------------------------------------------------------------------*/
    if (len > 0 && s->offset >= (uint32_t)TI_FWU_MANIFEST_SIZE)
    {
        uint32_t imageOffset = s->offset;

        psaRet = psa_fwu_write(s->targetSlot, imageOffset, pData, len);
        if (psaRet != PSA_SUCCESS)
        {
            UART_PRINT("[OTA] psa_fwu_write(%d, offset=%lu, len=%lu) failed: %d\r\n",
                       s->targetSlot,
                       (unsigned long)imageOffset,
                       (unsigned long)len,
                       (int)psaRet);
            s->error = (int)psaRet;
            return -1;
        }

        s->offset += len;
    }

    return 0;
}

/* --------------------------------------------------------------------------
 * Job document parsing
 * --------------------------------------------------------------------------*/

/**
 * @brief Helper: extract a null-terminated string value from a JSON object.
 *
 * Searches for @p key in the JSON object at @p pObj / @p objLen and copies
 * the result (without quotes) into @p pOut.  Truncates to @p outMax - 1.
 *
 * @return @c 0 on success, @c -1 if the key is absent.
 */
static int json_get_string(const char *pObj, size_t objLen,
                           const char *key, size_t keyLen,
                           char *pOut, size_t outMax)
{
    char       *pVal    = NULL;
    size_t      valLen  = 0;
    JSONStatus_t jret;

    jret = JSON_SearchConst(pObj, objLen, key, keyLen,
                            (const char **)&pVal, &valLen, NULL);
    if (jret != JSONSuccess || pVal == NULL)
    {
        return -1;
    }

    size_t copyLen = (valLen < outMax - 1u) ? valLen : outMax - 1u;
    memcpy(pOut, pVal, copyLen);
    pOut[copyLen] = '\0';
    return 0;
}

/**
 * @brief Helper: extract a uint32 value from a JSON numeric field.
 *
 * @return @c 0 on success, @c -1 if the key is absent or non-numeric.
 */
static int json_get_uint32(const char *pObj, size_t objLen,
                           const char *key, size_t keyLen,
                           uint32_t *pOut)
{
    char        numBuf[16];
    char       *pVal   = NULL;
    size_t      valLen = 0;
    JSONStatus_t jret;

    jret = JSON_SearchConst(pObj, objLen, key, keyLen,
                            (const char **)&pVal, &valLen, NULL);
    if (jret != JSONSuccess || pVal == NULL)
    {
        return -1;
    }

    size_t copyLen = (valLen < sizeof(numBuf) - 1u) ? valLen : sizeof(numBuf) - 1u;
    memcpy(numBuf, pVal, copyLen);
    numBuf[copyLen] = '\0';

    *pOut = (uint32_t)strtoul(numBuf, NULL, 10);
    return 0;
}

/**
 * @brief Parse a received MQTT payload as an AWS IoT Jobs notification.
 *
 * Expected payload format (notify-next or $next/get/accepted):
 * @code
 * {
 *   "execution": {
 *     "jobId": "ota-vendor-v3.4.5-20260425",
 *     "jobDocument": {
 *       "components": [
 *         { "type":"Vendor_Image","slot1_id":4,"slot2_id":5,
 *           "url":"https://...","version":"3.4.5.0","size":1234567 }
 *       ]
 *     }
 *   }
 * }
 * @endcode
 *
 * @param[in]  pPayload  Raw MQTT PUBLISH payload.
 * @param[in]  payloadLen  Byte count.
 * @param[out] pJob  Destination for parsed job data.
 *
 * @return @c 0 on success, @c -1 on parse error or empty components array.
 */
static int parse_job_document(const char *pPayload, size_t payloadLen,
                              ota_job_t *pJob)
{
    char       *pJobId    = NULL;
    size_t      jobIdLen  = 0;
    char       *pComps    = NULL;
    size_t      compsLen  = 0;
    JSONTypes_t compType;
    JSONStatus_t jret;

    memset(pJob, 0, sizeof(*pJob));

    /* ---- Extract jobId ---- */
    jret = JSON_SearchConst(pPayload, payloadLen,
                            "execution.jobId", 15,
                            (const char **)&pJobId, &jobIdLen, NULL);
    if (jret != JSONSuccess || pJobId == NULL)
    {
        UART_PRINT("[OTA] Job document: execution.jobId not found\r\n");
        return -1;
    }
    size_t idCopy = (jobIdLen < OTA_MAX_JOB_ID_LEN - 1u) ?
                    jobIdLen : OTA_MAX_JOB_ID_LEN - 1u;
    memcpy(pJob->jobId, pJobId, idCopy);
    pJob->jobId[idCopy] = '\0';

    UART_PRINT("[OTA] Job ID: %s\r\n", pJob->jobId);

    /* ---- Extract components array ---- */
    jret = JSON_SearchConst(pPayload, payloadLen,
                            "execution.jobDocument.components", 32,
                            (const char **)&pComps, &compsLen, &compType);
    if (jret != JSONSuccess || pComps == NULL || compType != JSONArray)
    {
        UART_PRINT("[OTA] Job document: components array not found\r\n");
        return -1;
    }

    /* ---- Iterate array items ---- */
    size_t start = 0, next = 0;
    JSONPair_t pair = { 0 };

    while (pJob->numComponents < OTA_MAX_COMPONENTS)
    {
        jret = JSON_Iterate(pComps, compsLen, &start, &next, &pair);
        if (jret == JSONNotFound) { break; }
        if (jret != JSONSuccess)
        {
            UART_PRINT("[OTA] JSON_Iterate error: %d\r\n", (int)jret);
            return -1;
        }
        if (pair.value == NULL || pair.valueLength == 0) { continue; }

        ota_component_t *comp = &pJob->components[pJob->numComponents];

        /* type */
        if (json_get_string(pair.value, pair.valueLength,
                            "type", 4,
                            comp->type, sizeof(comp->type)) != 0)
        {
            UART_PRINT("[OTA] Component missing 'type'\r\n");
            return -1;
        }

        /* slot1_id */
        uint32_t tmpVal = 0;
        if (json_get_uint32(pair.value, pair.valueLength,
                            "slot1_id", 8, &tmpVal) != 0)
        {
            UART_PRINT("[OTA] Component missing 'slot1_id'\r\n");
            return -1;
        }
        comp->slot1_id = (uint8_t)tmpVal;

        /* slot2_id */
        if (json_get_uint32(pair.value, pair.valueLength,
                            "slot2_id", 8, &tmpVal) != 0)
        {
            UART_PRINT("[OTA] Component missing 'slot2_id'\r\n");
            return -1;
        }
        comp->slot2_id = (uint8_t)tmpVal;

        /* url */
        if (json_get_string(pair.value, pair.valueLength,
                            "url", 3,
                            comp->url, sizeof(comp->url)) != 0)
        {
            UART_PRINT("[OTA] Component missing 'url'\r\n");
            return -1;
        }

        /* version (optional — set to "?" if absent) */
        if (json_get_string(pair.value, pair.valueLength,
                            "version", 7,
                            comp->version, sizeof(comp->version)) != 0)
        {
            snprintf(comp->version, sizeof(comp->version), "?");
        }

        /* size (optional — informational) */
        if (json_get_uint32(pair.value, pair.valueLength,
                            "size", 4, &comp->size) != 0)
        {
            comp->size = 0;
        }

        UART_PRINT("[OTA] Component[%u]: type=%s slot1=%u slot2=%u ver=%s size=%lu\r\n",
                   pJob->numComponents,
                   comp->type, comp->slot1_id, comp->slot2_id,
                   comp->version, (unsigned long)comp->size);

        pJob->numComponents++;
    }

    if (pJob->numComponents == 0)
    {
        UART_PRINT("[OTA] Job document: no valid components found\r\n");
        return -1;
    }

    return 0;
}

/* --------------------------------------------------------------------------
 * Job status update
 * --------------------------------------------------------------------------*/

/**
 * @brief Publish a job status update to AWS IoT Jobs.
 *
 * Publishes to @c $aws/things/{name}/jobs/{jobId}/update at QoS 0
 * (fire-and-forget; no response handling needed for the device's purposes).
 *
 * @param[in] jobId   Null-terminated job ID string.
 * @param[in] status  "SUCCEEDED", "FAILED", or "IN_PROGRESS".
 */
static void publish_job_status(const char *jobId, const char *status)
{
    char topicBuf[144];
    char payloadBuf[64];
    MQTTPublishInfo_t pub;

    if (s_mqttCtx == NULL || jobId == NULL || jobId[0] == '\0')
    {
        return;
    }

    snprintf(topicBuf, sizeof(topicBuf),
             "$aws/things/%s/jobs/%s/update", s_thingName, jobId);

    snprintf(payloadBuf, sizeof(payloadBuf), "{\"status\":\"%s\"}", status);

    memset(&pub, 0, sizeof(pub));
    pub.qos             = MQTTQoS0;
    pub.pTopicName      = topicBuf;
    pub.topicNameLength = (uint16_t)strlen(topicBuf);
    pub.pPayload        = payloadBuf;
    pub.payloadLength   = strlen(payloadBuf);

    MQTTStatus_t ret = MQTT_Publish(s_mqttCtx, &pub, 0U);
    if (ret != MQTTSuccess)
    {
        UART_PRINT("[OTA] Failed to publish job status '%s': %d\r\n",
                   status, (int)ret);
    }
    else
    {
        UART_PRINT("[OTA] Job %s status: %s\r\n", jobId, status);
    }
}

/* --------------------------------------------------------------------------
 * Helper: next OTA packet ID
 * --------------------------------------------------------------------------*/

static uint16_t next_packet_id(void)
{
    uint16_t id = s_packetId++;
    if (s_packetId == 0U)
    {
        s_packetId = OTA_MQTT_PACKET_ID_BASE;
    }
    return id;
}

/* --------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/

int AwsIotOta_Init(MQTTContext_t *pMqttCtx)
{
    const char *thingName;

    if (pMqttCtx == NULL)
    {
        UART_PRINT("[OTA] Init failed: NULL MQTT context\r\n");
        return -1;
    }

    thingName = AwsIot_GetThingName();
    if (thingName == NULL || thingName[0] == '\0')
    {
        UART_PRINT("[OTA] Init failed: thing name unavailable\r\n");
        return -1;
    }

    s_mqttCtx = pMqttCtx;
    snprintf(s_thingName, sizeof(s_thingName), "%s", thingName);

    /* Pre-build topic strings */
    snprintf(s_topicNotifyNext, sizeof(s_topicNotifyNext),
             "$aws/things/%s/jobs/notify-next", s_thingName);
    snprintf(s_topicNextGet, sizeof(s_topicNextGet),
             "$aws/things/%s/jobs/$next/get", s_thingName);
    snprintf(s_topicNextGetAccepted, sizeof(s_topicNextGetAccepted),
             "$aws/things/%s/jobs/$next/get/accepted", s_thingName);

    s_update_pending = false;
    memset(&s_job, 0, sizeof(s_job));

    UART_PRINT("[OTA] Initialised for thing: %s\r\n", s_thingName);
    return 0;
}

int AwsIotOta_Subscribe(void)
{
    MQTTSubscribeInfo_t subs[2];
    MQTTStatus_t        ret;

    if (s_mqttCtx == NULL)
    {
        UART_PRINT("[OTA] Subscribe failed: not initialised\r\n");
        return -1;
    }

    /* notify-next: pushed by AWS IoT when a new job is available */
    subs[0].qos               = MQTTQoS0;
    subs[0].pTopicFilter      = s_topicNotifyNext;
    subs[0].topicFilterLength = (uint16_t)strlen(s_topicNotifyNext);

    /* $next/get/accepted: response to our poll requests */
    subs[1].qos               = MQTTQoS0;
    subs[1].pTopicFilter      = s_topicNextGetAccepted;
    subs[1].topicFilterLength = (uint16_t)strlen(s_topicNextGetAccepted);

    ret = MQTT_Subscribe(s_mqttCtx, subs, 2u, next_packet_id());
    if (ret != MQTTSuccess)
    {
        UART_PRINT("[OTA] MQTT_Subscribe failed: %d\r\n", (int)ret);
        return -1;
    }

    /* Drive the process loop once to receive the SUBACK */
    MQTT_ProcessLoop(s_mqttCtx);

    UART_PRINT("[OTA] Subscribed to Jobs topics\r\n");
    return 0;
}

int AwsIotOta_CheckForUpdate(void)
{
    MQTTPublishInfo_t pub;
    MQTTStatus_t      ret;

    if (s_mqttCtx == NULL)
    {
        return -1;
    }

    memset(&pub, 0, sizeof(pub));
    pub.qos             = MQTTQoS0;
    pub.pTopicName      = s_topicNextGet;
    pub.topicNameLength = (uint16_t)strlen(s_topicNextGet);
    pub.pPayload        = "{}";
    pub.payloadLength   = 2u;

    ret = MQTT_Publish(s_mqttCtx, &pub, 0U);
    if (ret != MQTTSuccess)
    {
        UART_PRINT("[OTA] CheckForUpdate publish failed: %d\r\n", (int)ret);
        return -1;
    }

    UART_PRINT("[OTA] Polling for pending jobs...\r\n");
    return 0;
}

void AwsIotOta_OnMqttPublish(MQTTPublishInfo_t *pPublish)
{
    const char *pTopic;
    uint16_t    topicLen;

    if (pPublish == NULL) { return; }

    pTopic   = pPublish->pTopicName;
    topicLen = pPublish->topicNameLength;

    if (pTopic == NULL || topicLen == 0) { return; }

    /* Check if this publish is for one of our subscribed Jobs topics */
    bool isNotifyNext = (topicLen == strlen(s_topicNotifyNext) &&
                         strncmp(pTopic, s_topicNotifyNext, topicLen) == 0);
    bool isGetAccepted = (topicLen == strlen(s_topicNextGetAccepted) &&
                          strncmp(pTopic, s_topicNextGetAccepted, topicLen) == 0);

    if (!isNotifyNext && !isGetAccepted)
    {
        return; /* Not an OTA topic */
    }

    if (pPublish->pPayload == NULL || pPublish->payloadLength == 0)
    {
        /* Empty payload on notify-next means no pending jobs */
        UART_PRINT("[OTA] No pending jobs\r\n");
        return;
    }

    UART_PRINT("[OTA] Job notification received (%u bytes)\r\n",
               (unsigned)pPublish->payloadLength);

    if (parse_job_document((const char *)pPublish->pPayload,
                           pPublish->payloadLength, &s_job) == 0)
    {
        s_update_pending = true;
        UART_PRINT("[OTA] Job parsed: %u component(s) pending\r\n",
                   s_job.numComponents);
    }
    else
    {
        UART_PRINT("[OTA] Failed to parse job document\r\n");
    }
}

bool AwsIotOta_IsUpdatePending(void)
{
    return s_update_pending;
}

/**
 * @brief Parse a dotted version string "major.minor.patch.build" into a
 *        @c psa_fwu_image_version_t.
 *
 * @return @c 0 on success, @c -1 if the string does not match the expected
 *         four-part format.
 */
static int s_parse_version(const char *vStr, psa_fwu_image_version_t *pVer)
{
    unsigned int major, minor, patch;
    unsigned int build;

    if (sscanf(vStr, "%u.%u.%u.%u", &major, &minor, &patch, &build) != 4)
    {
        return -1;
    }
    pVer->major = (uint8_t)major;
    pVer->minor = (uint8_t)minor;
    pVer->patch = (uint16_t)patch;
    pVer->build = (uint32_t)build;
    return 0;
}

int AwsIotOta_ExecuteUpdate(void)
{
    uint8_t i;
    int     ret = 0;

    if (!s_update_pending || s_job.numComponents == 0)
    {
        UART_PRINT("[OTA] ExecuteUpdate called with no pending job\r\n");
        return -1;
    }

    UART_PRINT("[OTA] Starting update: job=%s, components=%u\r\n",
               s_job.jobId, s_job.numComponents);

    publish_job_status(s_job.jobId, "IN_PROGRESS");

    /* ----------------------------------------------------------------
     * Download + write each component
     * ----------------------------------------------------------------*/
    for (i = 0; i < s_job.numComponents; i++)
    {
        ota_component_t    *comp = &s_job.components[i];
        psa_fwu_component_t targetSlot;
        psa_fwu_image_version_t primaryVer;
        uint32_t            bytesReceived = 0;

        UART_PRINT("[OTA] Component %u/%u: %s (ver=%s, size=%lu)\r\n",
                   i + 1u, s_job.numComponents,
                   comp->type, comp->version, (unsigned long)comp->size);

        /* Select the non-primary target slot */
        if (s_fwu_selectTargetSlot(comp->slot1_id, comp->slot2_id,
                                   &targetSlot, &primaryVer) != 0)
        {
            UART_PRINT("[OTA] selectTargetSlot failed for %s\r\n", comp->type);
            ret = -1;
            goto fail;
        }

        /* Check whether the primary slot already runs the target version.
         * This happens when the job SUCCEEDED status was not delivered before
         * the OTA reboot (QoS 0, 500 ms window) and AWS re-delivers the job
         * on reconnect.  Publishing SUCCEEDED here closes the job without a
         * redundant download. */
        {
            psa_fwu_image_version_t jobVer;
            if (s_parse_version(comp->version, &jobVer) == 0 &&
                primaryVer.major == jobVer.major &&
                primaryVer.minor == jobVer.minor &&
                primaryVer.patch == jobVer.patch &&
                primaryVer.build == jobVer.build)
            {
                UART_PRINT("[OTA] %s already at v%s — publishing SUCCEEDED\r\n",
                           comp->type, comp->version);
                publish_job_status(s_job.jobId, "SUCCEEDED");
                s_update_pending = false;
                return 0;
            }
        }

        /* Prepare slot for writing */
        if (s_fwu_prepareSlot(targetSlot) != 0)
        {
            UART_PRINT("[OTA] prepareSlot(%d) failed for %s\r\n",
                       (int)targetSlot, comp->type);
            ret = -1;
            goto fail;
        }

        /* Set up download context */
        ota_dl_ctx_t dlCtx;
        memset(&dlCtx, 0, sizeof(dlCtx));
        dlCtx.targetSlot = targetSlot;

        /* Download + stream to FWU */
        ota_https_req_t req;
        req.pURL      = comp->url;
        req.pCaCert   = democonfigROOT_CA_PEM;
        req.caCertLen = strlen(democonfigROOT_CA_PEM);
        req.dataCb    = s_fwu_write_cb;
        req.pUserCtx  = &dlCtx;

        UART_PRINT("[OTA] Downloading %s...\r\n", comp->type);
        int dlRet = ota_https_download(&req, &bytesReceived);

        if (dlRet != 0 || dlCtx.error != 0)
        {
            UART_PRINT("[OTA] Download failed for %s (dlRet=%d, fwuErr=%d)\r\n",
                       comp->type, dlRet, dlCtx.error);
            psa_fwu_cancel(targetSlot);
            ret = -1;
            goto fail;
        }

        /* Mark component as CANDIDATE */
        psa_status_t psaRet = psa_fwu_finish(targetSlot);
        if (psaRet != PSA_SUCCESS)
        {
            UART_PRINT("[OTA] psa_fwu_finish(%d) failed: %d\r\n",
                       (int)targetSlot, (int)psaRet);
            psa_fwu_cancel(targetSlot);
            ret = -1;
            goto fail;
        }

        UART_PRINT("[OTA] %s: %lu bytes → CANDIDATE\r\n",
                   comp->type, (unsigned long)bytesReceived);
    }

    /* ----------------------------------------------------------------
     * Install all CANDIDATEs at once → STAGED
     * ----------------------------------------------------------------*/
    UART_PRINT("[OTA] Installing %u CANDIDATE(s)...\r\n", s_job.numComponents);
    psa_status_t installRet = psa_fwu_install();
    if (installRet != PSA_SUCCESS && installRet != PSA_SUCCESS_REBOOT)
    {
        UART_PRINT("[OTA] psa_fwu_install() failed: %d\r\n", (int)installRet);
        ret = -1;
        goto fail;
    }

    /* ----------------------------------------------------------------
     * All good — publish SUCCEEDED and reboot
     * ----------------------------------------------------------------*/
    publish_job_status(s_job.jobId, "SUCCEEDED");
    s_update_pending = false;

    UART_PRINT("[OTA] All components STAGED. Rebooting...\r\n");

    /* Small delay to allow the MQTT publish to be transmitted */
    vTaskDelay(500U / portTICK_PERIOD_MS);

    psa_fwu_request_reboot();
    /* Never reached if reboot succeeds */
    while (1) { vTaskDelay(100U / portTICK_PERIOD_MS); }

fail:
    publish_job_status(s_job.jobId, "FAILED");
    s_update_pending = false;
    return ret;
}

void AwsIotOta_HandleTrialState(void)
{
    int trialCount = 0;

    psa_fwu_init();
    s_fwu_scanPendingStates(&trialCount);

    if (trialCount > 0)
    {
        UART_PRINT("[OTA] %d component(s) in TRIAL — accepting\r\n", trialCount);
        psa_status_t ret = psa_fwu_accept();
        if (ret == PSA_SUCCESS || ret == PSA_SUCCESS_REBOOT)
        {
            /* A second reboot is required to permanently commit the accepted
             * image as PRIMARY.  Without it the bootloader sees the slot still
             * in TRIAL and rolls back on every power cycle. */
            UART_PRINT("[OTA] All TRIAL components accepted — rebooting to commit\r\n");
            vTaskDelay(100U / portTICK_PERIOD_MS);
            psa_fwu_request_reboot();
            while (1) { vTaskDelay(100U / portTICK_PERIOD_MS); }
        }
        else
        {
            UART_PRINT("[OTA] psa_fwu_accept() failed: %d\r\n", (int)ret);
        }
    }
    else
    {
        UART_PRINT("[OTA] No TRIAL components\r\n");
    }
}
