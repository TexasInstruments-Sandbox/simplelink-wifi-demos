# AWS IoT Plugin Architecture for CC3551 (SimpleLink Wi-Fi Demos)

## Overview

This document describes the **AWS IoT plugin architecture** for the CC3551 microcontroller within the **SimpleLink Wi-Fi Demos** project. The plugin provides a production-ready AWS IoT Core integration with support for:

- **X.509 Mutual TLS Authentication** — Certificate-based secure connection
- **MQTT Telemetry** — Periodic sensor data publishing
- **Device Shadow** — Remote device state synchronization and control
- **Device OTA** — AWS OTA implementation with full Jobs mechanism
- **coreMQTT Library** — FreeRTOS-compatible MQTT protocol stack
- **coreHTTP Library** — FreeRTOS-compatible HTTPS protocol stack

---

## Architecture Overview

### High-Level Component Stack

```
┌──────────────────────────────────────────────────────────────┐
│  Application Layer (User Code)                               │
│  ├─ AwsIotTelemetry_Connect/Run/Disconnect                   │
│  ├─ AwsIotLed_Subscribe/OnMqttPublish                        │
│  └─ Custom MQTT operations                                   │
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  AWS IoT Cloud Abstraction Layer                             │
│  Location: src/freertos/cloud/aws/                           │
│  ├─ aws_iot_telemetry.{c,h}    (Telemetry & subscription)    │
│  ├─ aws_iot_led.{c,h}          (Shadow-based LED control)    │
│  ├─ aws_iot_ota.{c,h}          (OTA abstraction layer)       │
│  ├─ aws_iot_ota_https.{c,h}    (HTTPS transport for OTA)     │
│  └─ aws_iot_mqtt.h             (Public API)                  │
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  coreMQTT Adaptation Layer (AWS-specific)                    │
│  Location: src/freertos/transport/                           │
│  ├─ aws_iot_core_mqtt.c        (MQTT wrapper for AWS)        │
│  └─ AwsIoTMQTT_Init()          (Initialize with QoS tracking)|
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  Transport Layer (TLS/TCP)                                   │
│  Location: src/freertos/transport/                           │
│  ├─ transport_tls_socket_using_mbedtls.c                     │
│  ├─ transport_socket.c         (Socket abstraction)          │
│  ├─ sockets_wrapper_lwip.c     (LwIP TCP/IP stack)           │
│  ├─ mbedtls_freertos_port.c    (mbedTLS thread safety)       │
│  └─ Transport configs                                        │
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  SimpleLink Wi-Fi SDK                                        │
│  Location: resources/simplelink-wifi-sdk/                    │
│  ├─ Wi-Fi drivers (CC3551 hardware)                          │
│  ├─ LwIP network stack integration                           │
│  └─ Hardware crypto acceleration (PSA Crypto)               │
└──────────────────────────────────────────────────────────────┘
```

### Data Flow: Telemetry Publishing

```
┌──────────────┐
│   Sensors    │  (Temperature, Humidity via I2C)
└──────┬───────┘
       │ AwsIotTelemetry_Init()
       ▼
┌──────────────────────────┐
│  JSON Telemetry Payload  │  (e.g., {"temp": 25.5})
│  (512 bytes max)         │
└──────┬───────────────────┘
       │ AwsIotTelemetry_Run() every 10s
       ▼
┌──────────────────────────────────────────────┐
│  MQTT_Publish(topic, payload, QoS=1)         │
│  Topic: "AwsTI/{thingName}/telemetry"        │
└──────┬───────────────────────────────────────┘
       │ coreMQTT stack (aws_iot_core_mqtt.c)
       ▼
┌──────────────────────────────────────────────┐
│  TLS Encrypt & TCP Send                      │
│  (mTLS handshake on first connect)           │
└──────┬───────────────────────────────────────┘
       │
       ▼
 AWS IoT Core (MQTT Broker)
```

### Data Flow: Remote LED Control (Device Shadow)

```
 AWS IoT Core (Device Shadow Service)
       │
       │ User updates desired state:
       │ {"state": {"desired": {"green_led": "on", "blue_led": "off", "red_led": "off"}}}
       │
       ▼
┌────────────────────────────┐
│  Shadow Delta Topic        │
│  $aws/things/{name}/       │
│  shadow/update/delta       │
└────────┬───────────────────┘
         │ TLS/TCP decrypt
         ▼
┌────────────────────────────────────────────┐
│  MQTT_ProcessLoop() receives PUBLISH       │
│  coreMQTT callback invokes user handler    │
└────────┬───────────────────────────────────┘
         │ AwsIotLed_OnMqttPublish()
         ▼
┌────────────────────────────────────────────┐
│  Parse JSON delta: {green_led, blue_led,   │
│  red_led states}                           │
└────────┬───────────────────────────────────┘
         │
         ▼
┌────────────────────────────────────────────┐
│  Control GPIO (LEDs on/off)                │
└────────┬───────────────────────────────────┘
         │
         ▼
┌────────────────────────────────────────────────┐
│  Publish updated state to shadow/update topic  │
│  Reported: {green_led, blue_led, red_led}     │
└────────────────────────────────────────────────┘
```

---

## Directory Structure

```
aws-osprey/
├── simplelink_wi-fi_demos/
│   ├── README.md                                  # Main project README
│   ├── .gitmodules                               # Submodule definitions
│   ├── Makefile                                  # Root-level build system
│   ├── imports.mak                               # Build tool paths
│   │
│   ├── projects/
│   │   └── LP_EM_CC35X1/
│   │       └── (Example projects go here)
│   │
│   ├── src/freertos/
│   │   │
│   │   ├── cloud/
│   │   │   └── aws/                              # AWS Cloud Abstraction
│   │   │       ├── aws_iot_telemetry.c           # Telemetry publisher
│   │   │       ├── aws_iot_telemetry.h
│   │   │       ├── aws_iot_led.c                 # Shadow-based LED control
│   │   │       ├── aws_iot_led.h
│   │   │       ├── aws_iot_ota.c                 # ota wrapper
│   │   │       ├── aws_iot_ota.h
│   │   │       ├── aws_iot_ota_https.c           # ota HTTPS wrapper
│   │   │       ├── aws_iot_ota_https.h
│   │   │       └── aws_iot_mqtt.h                # Public AWS API
│   │   │
│   │   ├── transport/
│   │   │   ├── aws/                              # AWS-specific adaptation
│   │   │   │   └── (TLS/MQTT configs for AWS)
│   │   │   ├── aws_iot_core_mqtt.c               # MQTT wrapper
│   │   │   ├── transport_tls_socket_using_mbedtls.c
│   │   │   ├── transport_socket.c
│   │   │   ├── sockets_wrapper_lwip.c
│   │   │   ├── mbedtls_freertos_port.c
│   │   │   ├── mbedtls_freertos_port.h
│   │   │   └── configs/
│   │   │       └── (Transport config headers)
│   │   │
│   │   ├── ns/                                   # Network stack abstraction
│   │   │   └── (DNS, TCP/IP, Wi-Fi interfaces)
│   │   │
│   │   ├── logging/                              # Logging utilities
│   │   │
│   │   └── platform/                             # Platform-specific code
│   │
│   ├── resources/
│   │   ├── simplelink-wifi-sdk/                  # TI SDK (git submodule)
│   │   │   ├── (CC3551 drivers, crypto, HAL)
│   │   │   └── (Wi-Fi firmware, LwIP integration)
│   │   │
│   │   └── third-party/                          # External libraries
│   │       ├── coreMQTT/                         # MQTT 3.1.1 protocol
│   │       ├── coreJSON/                         # JSON parsing/generation
│   │       ├── backoffAlgorithm/                 # Reconnection backoff
│   │       └── coreHTTP/                         # (Future HTTP support)
│   │
│   └── (Additional utility directories)
```

---

## Key Components

### 1. AWS IoT Telemetry Module (`aws_iot_telemetry.{c,h}`)

**Purpose:** Connects to AWS IoT Core and periodically publishes sensor telemetry.

**Key Functions:**
- `AwsIotTelemetry_Init()` — Initialize I2C sensors and telemetry system
- `AwsIotTelemetry_Connect()` — Establish mTLS MQTT connection to AWS
- `AwsIotTelemetry_Run()` — Blocking loop that publishes telemetry every 10 seconds
- `AwsIotTelemetry_Disconnect()` — Clean up MQTT and TLS connections
- `AwsIotTelemetry_GetMqttCtx()` — Get shared MQTT context (for LED module, OTA, etc.)
- `AwsIotTelemetry_RegisterPublishCallback()` — Subscribe to incoming messages
- `AwsIotTelemetry_RegisterTickCallback()` — Periodic non-blocking operations

**Configuration:**
- **Telemetry Interval:** `AWS_IOT_TELEMETRY_PERIOD_MS` (default: 10000 ms)
- **Topic Prefix:** `AWS_IOT_TELEMETRY_TOPIC_PREFIX` (default: "AwsTI")
- **JSON Payload Size:** `AWS_IOT_TELEMETRY_JSON_MAX_LEN` (512 bytes)

**Typical Usage:**
```c
// After WiFi connection
AwsIotTelemetry_Init();          // Initialize sensors
AwsIotTelemetry_Connect();       // Connect to AWS IoT Core
AwsIotTelemetry_Run();           // Blocking telemetry loop (returns on error)
AwsIotTelemetry_Disconnect();    // Clean shutdown
```

### 2. AWS IoT LED Shadow Module (`aws_iot_led.{c,h}`)

**Purpose:** Remote LED control via AWS IoT Device Shadow (desired ↔ reported states).

**Key Functions:**
- `AwsIotLed_Init()` — Cache MQTT context and build shadow topic strings
- `AwsIotLed_Subscribe()` — Subscribe to shadow delta and state topics
- `AwsIotLed_RequestCurrentState()` — Fetch current shadow from AWS
- `AwsIotLed_OnMqttPublish()` — Handle incoming shadow PUBLISH messages

**Shadow Topics:**
- **Delta (desired ≠ reported):** `$aws/things/{name}/shadow/update/delta`
- **Get accepted (response):** `$aws/things/{name}/shadow/get/accepted`
- **Update (publish state):** `$aws/things/{name}/shadow/update`

**Shadow Document Format:**
```json
{
  "state": {
    "desired": {
      "green_led": "on",
      "blue_led": "off",
      "red_led": "off"
    },
    "reported": {
      "green_led": "on",
      "blue_led": "off",
      "red_led": "off"
    }
  }
}
```

**Typical Usage:**
```c
// After AwsIotTelemetry_Connect() succeeds
MQTTContext_t *pCtx = AwsIotTelemetry_GetMqttCtx();
AwsIotLed_Init(pCtx);
AwsIotLed_Subscribe();
AwsIotLed_RequestCurrentState();

// Register with telemetry callback dispatcher
AwsIotTelemetry_RegisterPublishCallback(AwsIotLed_OnMqttPublish);
```

### 3. MQTT Adaptation Layer (`aws_iot_core_mqtt.c`)

**Purpose:** Wraps coreMQTT for AWS IoT, handling QoS state tracking.

**Key Functions:**
- `AwsIoTMQTT_Init()` — Initialize MQTT context with stateful QoS support
  - Sets up outgoing publish records (15 slots)
  - Sets up incoming publish records (15 slots)
  - Enables QoS 1/2 acknowledgment tracking

**Config:**
- `mqttexampleOUTGOING_PUBLISH_RECORD_LEN` = 15
- `mqttexampleINCOMING_PUBLISH_RECORD_LEN` = 15

### 4. Transport Layer (`src/freertos/transport/`)

**Components:**
- **`transport_tls_socket_using_mbedtls.c`** — TLS handshake and encryption (mTLS)
- **`transport_socket.c`** — TCP socket abstraction
- **`sockets_wrapper_lwip.c`** — LwIP TCP/IP stack integration
- **`mbedtls_freertos_port.c`** — Thread-safe mbedTLS for FreeRTOS

**Features:**
- X.509 certificate verification
- Mutual TLS (client certificate + server certificate validation)
- Hardware crypto acceleration via PSA Crypto (CC3551 support)
- FreeRTOS thread safety (mutexes for concurrent TLS operations)

---

## AWS IoT Core Setup: Tokens, Topics, Policies

### 1. Prerequisites

Before connecting your CC3551 device, you must set up AWS IoT Core resources:

1. **AWS Account** with IoT Core access
2. **AWS CLI** configured with credentials
3. **Device Certificate & Private Key** (X.509)
4. **Root CA Certificate** from AWS
5. **IoT Thing** representing your device
6. **IoT Policy** restricting device permissions

---

### 2. Step-by-Step AWS IoT Console Setup

#### Step 1: Create a Certificate

**Via AWS IoT Console:**

1. Navigate to [AWS IoT Console](https://console.aws.amazon.com/iot/home)
2. Go to **Certificates** (left sidebar under Manage -> Security)
3. Click **Add certificate -> Create certificate**
4. Select **Auto-generate new certificate** and **Active** Options
5. Click **Create** at the bottom
6. Download files:
   - `certificate.pem`
   - `private.key`
   - `public.key`
   - `AmazonRootCA1.pem` (or copy from [AWS Trust Services](https://www.amazontrust.com/repository/AmazonRootCA1.pem))
7. From the certificates page, find the newly created certificate and **Copy the Certificate ARN** (e.g., `arn:aws:iot:us-east-1:123456789012:cert/abcd1234...`)

**Via AWS CLI:**

```bash
# Create and activate a certificate
aws iot create-keys-and-certificate \
  --set-as-active \
  --certificate-pem-outfile certificate.pem \
  --private-key-outfile private.key \
  --public-key-outfile public.key

# Download root CA
wget https://www.amazontrust.com/repository/AmazonRootCA1.pem

# Save the certificateArn from the output
```

#### Step 2: Create an IoT Thing

**Via AWS IoT Console:**

1. Go to **Manage** → **Things** (left sidebar)
2. Click **Create thing**
3. Enter Thing Name: `DevMac_{MAC_ADDRESS}` (e.g., `DevMac_AABBCCDDEEFF`)
4. Click **Create**

**Via AWS CLI:**

```bash
aws iot create-thing --thing-name DevMac_AABBCCDDEEFF
```

#### Step 3: Attach Certificate to Thing

**Via AWS IoT Console:**

1. Open the certificate you created
2. Click **Actions** → **Attach thing**
3. Select `DevMac_AABBCCDDEEFF` (your thing name)
4. Click **Attach**

**Via AWS CLI:**

```bash
aws iot attach-thing-principal \
  --thing-name DevMac_AABBCCDDEEFF \
  --principal <certificateArn>
```

#### Step 4: Create an IoT Policy

**Via AWS IoT Console:**

1. Go to **Certificates** → **Policies** (or **Secure** → **Policies**)
2. Click **Create policy**
3. Enter Policy Name: `cc3551-policy`
4. Paste the policy document below
5. Click **Create**

**Policy Document (JSON):**

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:123456789012:client/DevMac_*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topic/AwsTI/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/$next/get",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/*/update"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/jobs/notify-next",
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/jobs/$next/get/accepted"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/notify-next",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/$next/get/accepted",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/*/update/accepted"
      ]
    }
  ]
}
```

**Replace:**
- `us-east-1` with your AWS region
- `123456789012` with your AWS Account ID
- `DevMac_*` allows any device with this naming pattern

**Via AWS CLI:**

```bash
# Create policy file
cat > policy.json <<'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:123456789012:client/DevMac_*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topic/AwsTI/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/$next/get",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/*/update"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/jobs/notify-next",
        "arn:aws:iot:us-east-1:123456789012:topicfilter/$aws/things/DevMac_*/jobs/$next/get/accepted"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/shadow/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/notify-next",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/$next/get/accepted",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/DevMac_*/jobs/*/update/accepted"
      ]
    }
  ]
}
EOF

# Create policy
aws iot create-policy \
  --policy-name cc3551-policy \
  --policy-document file://policy.json
```

#### Step 5: Attach Policy to Certificate

**Via AWS IoT Console:**

1. Open the certificate
2. Click **Actions** → **Attach policy**
3. Select `cc3551-policy`
4. Click **Attach**

**Via AWS CLI:**

```bash
aws iot attach-principal-policy \
  --policy-name cc3551-policy \
  --principal <certificateArn>
```

#### Step 6: Get Your AWS IoT Endpoint

**Via AWS IoT Console:**

1. Go to **Settings** (bottom-left)
2. Copy **Device data endpoint** (e.g., `abcd1234.iot.us-east-1.amazonaws.com`)

**Via AWS CLI:**

```bash
aws iot describe-endpoint --endpoint-type iot:Data-ATS
```

---

### 3. MQTT Topics and Payload Formats

#### Telemetry Topic

**Topic Pattern:** `AwsTI/{thingName}/telemetry`

**Example:** `AwsTI/DevMac_AABBCCDDEEFF/telemetry`

**Payload (JSON):**
```json
{
  "device_id": "DevMac_AABBCCDDEEFF",
  "timestamp_s": 1234567890,
  "temperature_c": "25.5",
  "axis_X": "-120",
  "axis_Y": "45",
  "axis_Z": "980",
  "battery_mv": 3700,
  "sleep_pct": 40,
  "uptime_s": 3600,
  "rssi_dbm": -65,
  "app_version": "1.0.0"
}
```

**QoS:** 1 (At-least-once delivery)

---

#### Device Shadow Update Topic

**Topic Pattern:** `$aws/things/{thingName}/shadow/update`

**Payload (JSON - Device Reports LED State):**
```json
{
  "state": {
    "reported": {
      "green_led": "on",
      "blue_led": "off",
      "red_led": "off"
    }
  }
}
```

---

#### Device Shadow Delta Topic (Incoming)

**Topic Pattern:** `$aws/things/{thingName}/shadow/update/delta`

**Payload (JSON - what AWS sends when desired ≠ reported):**
```json
{
  "version": 1,
  "state": {
    "green_led": "on",
    "blue_led": "off",
    "red_led": "off"
  }
}
```

**Note:** AWS IoT includes additional metadata and timestamps in the full delta payload, but the device only parses the `state` fields.

---

#### Device Shadow Get Topic (Request)

**Topic Pattern:** `$aws/things/{thingName}/shadow/get`

**Request Payload:** `{}` (empty object)

**Response Topic:** `$aws/things/{thingName}/shadow/get/accepted`

**Response Payload (Full Shadow Document):**
```json
{
  "state": {
    "desired": {
      "green_led": "on",
      "blue_led": "off",
      "red_led": "off"
    },
    "reported": {
      "green_led": "on",
      "blue_led": "off",
      "red_led": "off"
    },
    "delta": {
      "green_led": "on"
    }
  },
  "metadata": {
    "desired": {
      "green_led": { "timestamp": 1619827200 },
      "blue_led": { "timestamp": 1619827200 },
      "red_led": { "timestamp": 1619827200 }
    },
    "reported": {
      "green_led": { "timestamp": 1619827100 },
      "blue_led": { "timestamp": 1619827100 },
      "red_led": { "timestamp": 1619827100 }
    },
    "delta": {
      "green_led": { "timestamp": 1619827200 }
    }
  },
  "version": 5,
  "timestamp": 1619827200
}
```

**Note:** Delta is only present if `desired` differs from `reported`. The device parses `state.desired.*` fields from this response.

---

#### IoT Jobs Topic (OTA Updates)

**Jobs Pattern:** `$aws/things/{thingName}/jobs/*`

**Job Notification Topic (Device receives):**
```
$aws/things/{thingName}/jobs/notify-next
```

**Get Next Job Topic (Device requests job details):**
```
$aws/things/{thingName}/jobs/$next/get
```

**Get Next Job Response Topic (AWS sends job info):**
```
$aws/things/{thingName}/jobs/$next/get/accepted
```

**Job Update Topic (Device reports job status):**
```
$aws/things/{thingName}/jobs/{jobId}/update
```

**Job Update Response Topic (AWS confirms status update):**
```
$aws/things/{thingName}/jobs/{jobId}/update/accepted
```

**OTA Flow 1: Polling (On Startup)**

Device checks for pending jobs immediately after connecting to AWS:

1. Device publishes (empty) request to `$aws/things/{thingName}/jobs/$next/get`
2. AWS responds on `$aws/things/{thingName}/jobs/$next/get/accepted` with:
   - Full job document (if job exists)
   - Empty response (if no jobs pending)
3. If job exists:
   - Device downloads firmware from pre-signed S3 URL (separate HTTPS connection)
   - Device writes firmware to secondary flash slot (PSA FWU)
   - Device publishes job status update to `$aws/things/{thingName}/jobs/{jobId}/update`
   - AWS confirms on `$aws/things/{thingName}/jobs/{jobId}/update/accepted`
   - Device reboots with new firmware

**OTA Flow 2: Push Notification (While Running)**

AWS proactively sends job notification when new job becomes available:

1. Device subscribes to `$aws/things/{thingName}/jobs/notify-next` (at startup)
2. While device is running, AWS publishes job notification to `notify-next` with:
   - Full job document
   - Execution details
3. Device receives notification and:
   - Downloads firmware from pre-signed S3 URL (separate HTTPS connection)
   - Writes firmware to secondary flash slot (PSA FWU)
   - Publishes job status update to `$aws/things/{thingName}/jobs/{jobId}/update`
   - AWS confirms on `$aws/things/{thingName}/jobs/{jobId}/update/accepted`
   - Device reboots with new firmware

**Key Differences:**

| Aspect | Flow 1 (Polling) | Flow 2 (Push) |
|--------|-----------------|--------------|
| **Triggered by** | Device (on startup) | AWS (when job available) |
| **Responsiveness** | Periodic check (only at startup or intervals) | Real-time (immediate notification) |
| **Payload** | Empty request, full response | Full job document in notification |
| **Use case** | Startup check, periodic polling | Continuous monitoring for new jobs |
| **Subscription** | Not needed for polling | Required for receiving pushes |

Both flows are supported by the device code—it handles jobs from either topic.

---

### 4. Policy Permissions Explained

| Action | Resource | Purpose |
|--------|----------|---------|
| `iot:Connect` | `client/{thingName}` | Allow device to connect with this client ID |
| `iot:Publish` | `topic/AwsTI/*` | Allow publishing telemetry to custom telemetry topic |
| `iot:Publish` | `topic/$aws/things/.../shadow/*` | Allow publishing shadow state updates |
| `iot:Publish` | `topic/$aws/things/.../jobs/$next/get` | Allow requesting next OTA job |
| `iot:Publish` | `topic/$aws/things/.../jobs/*/update` | Allow publishing job status updates |
| `iot:Subscribe` | `topicfilter/$aws/things/.../shadow/*` | Allow subscribing to shadow topics (desired, delta, etc.) |
| `iot:Subscribe` | `topicfilter/$aws/things/.../jobs/notify-next` | Allow subscribing to job notifications |
| `iot:Subscribe` | `topicfilter/$aws/things/.../jobs/$next/get/accepted` | Allow subscribing to job details response |
| `iot:Receive` | `topic/$aws/things/.../shadow/*` | Allow receiving shadow messages |
| `iot:Receive` | `topic/$aws/things/.../jobs/notify-next` | Allow receiving job notifications |
| `iot:Receive` | `topic/$aws/things/.../jobs/$next/get/accepted` | Allow receiving job details from AWS |
| `iot:Receive` | `topic/$aws/things/.../jobs/*/update/accepted` | Allow receiving job status confirmation |

**Principle of Least Privilege:**
- Restrict to only necessary topics
- Don't use wildcard `*` unless required
- Separate policies for read vs. write operations

---

### 5. Security Checklist

| Item | Status | Details |
|------|--------|---------|
| **Certificate** | ✓ | X.509 issued by AWS, activated |
| **Private Key** | ✓ | Stored securely on device (NVOCMP flash) |
| **Root CA** | ✓ | Downloaded from AWS Trust Services |
| **Thing Created** | ✓ | Device representation in IoT console |
| **Policy Attached** | ✓ | Least-privilege permissions applied |
| **Certificate Attached to Thing** | ✓ | Certificate linked to Thing |
| **Endpoint Noted** | ✓ | Device data endpoint copied |
| **TLS 1.2+** | ✓ | mbedTLS enforces TLS 1.2 minimum |
| **Mutual TLS** | ✓ | Client cert presented, server cert verified |

---


## Testing the Connection

### Option 1: AWS IoT Console Test Client (Easiest)

**No prerequisites — everything in the AWS Console:**

1. Go to [AWS IoT Console](https://console.aws.amazon.com/iot/home)
2. Left sidebar → **Test** → **MQTT Test client**
3. **Subscribe to topics:**
   - In the **Subscribe to a topic** field, enter: `AwsTI/DevMac_AABBCCDDEEFF/telemetry`
   - Click **Subscribe**
   - Incoming telemetry will appear in real-time

4. **View Device Shadow:**
   - Go to **Manage** → **Things** → **DevMac_AABBCCDDEEFF** → **Shadow**
   - View current state, or edit desired state to control LEDs

5. **Publish to shadow:**
   - In the Test client, **Publish to a topic:** `$aws/things/DevMac_AABBCCDDEEFF/shadow/update`
   - Payload:
     ```json
     {"state":{"desired":{"green_led":"on","blue_led":"off","red_led":"off"}}}
     ```
   - Click **Publish**

6. **Test OTA Job Creation:**
   - Go to **Manage** → **Things** → **DevMac_AABBCCDDEEFF** → **Jobs**
   - Click **Create job**
   - Choose **"Create a custom job"** (or **"Create OTA update job"** if available)
   - Upload your firmware binary (or choose file from S3)
   - Skip "Sign a new file" for testing (not required for development)
   - Create the job and deploy to device
   - Subscribe to job topics to monitor:
     - `$aws/things/DevMac_AABBCCDDEEFF/jobs/notify-next` (job notifications)
     - `$aws/things/DevMac_AABBCCDDEEFF/jobs/$next/get/accepted` (job details)
   - Watch device download firmware and reboot

**Advantages:**
- No external tools needed
- Real-time message viewing
- No certificate file management
- Visual shadow editor
- Easy job creation and monitoring

---

### Option 2: AWS CLI (Command Line)

**Prerequisites:**
- AWS CLI installed and configured

**Test Device Shadow Control:**
```bash
# Update shadow desired state (turn green LED on)
aws iot-data update-thing-shadow \
  --thing-name DevMac_AABBCCDDEEFF \
  --payload '{"state":{"desired":{"green_led":"on","blue_led":"off","red_led":"off"}}}' \
  shadow.json

# View shadow state
aws iot-data get-thing-shadow \
  --thing-name DevMac_AABBCCDDEEFF \
  shadow.json

cat shadow.json
```

**Advantages:**
- No certificate files needed
- Uses AWS credentials from `~/.aws/config`
- Scriptable and automatable
- Shadow-specific operations (not general MQTT)

---

### Option 3: Mosquitto CLI (Linux/macOS)

**Prerequisites:**
- `mosquitto_pub` installed (`apt-get install mosquitto-clients`)
- Device certificates downloaded locally
- Device must support mTLS connection (for testing from external machine)

**Test Telemetry Publishing:**
```bash
# Subscribe to telemetry topic (in one terminal)
mosquitto_sub \
  --cert certificate.pem \
  --key private.key \
  --cafile AmazonRootCA1.pem \
  -h abcd1234.iot.us-east-1.amazonaws.com \
  -p 8883 \
  -t 'AwsTI/DevMac_AABBCCDDEEFF/telemetry'

# In another terminal, publish a test message
mosquitto_pub \
  --cert certificate.pem \
  --key private.key \
  --cafile AmazonRootCA1.pem \
  -h abcd1234.iot.us-east-1.amazonaws.com \
  -p 8883 \
  -t 'AwsTI/DevMac_AABBCCDDEEFF/telemetry' \
  -m '{"device_id":"DevMac_AABBCCDDEEFF","timestamp_s":1234567890,"temperature_c":"25.5","axis_X":"-120","axis_Y":"45","axis_Z":"980","battery_mv":3700,"sleep_pct":40,"uptime_s":3600,"rssi_dbm":-65,"app_version":"1.0.0"}'
```

**Advantages:**
- Full MQTT protocol support
- Direct broker connection
- Low-level control

---

### Recommended Testing Flow

1. **Verify connectivity** → Check telemetry publishing (subscribe to `AwsTI/DevMac_*/telemetry`)
2. **Test remote control** → Use AWS Console Test to control LEDs via Device Shadow
3. **Test OTA update** → Create a job in AWS Console, upload firmware, deploy to device
4. **Monitor OTA progress** → Subscribe to job topics and watch device download + reboot
5. **Advanced MQTT testing** → Use mosquitto_pub/mosquitto_sub for low-level protocol testing

---

## AWS IoT Console Features

### 1. Monitor Device Activity

**Test (left sidebar):**
- Click on a topic to subscribe and monitor messages in real-time
- See telemetry, shadow updates, and errors

### 2. Device Shadow Management

**Manage → Things → {thingName} → Shadow:**
- View current shadow state
- Edit desired state (to control device)
- See reported state from device

---

## AWS IoT Jobs: Full OTA Firmware Update Process

This section describes the complete end-to-end process for deploying firmware updates to CC3551 devices using AWS IoT Jobs and S3 storage.

### Context

The flash used in the LP-EM-CC35X1 is IS25WJ064F (8 MB) and `Mem_cfg.ota = true` is now
active — dual A/B slots are provisioned and the device is ready for OTA.  AWS IoT Jobs is the right mechanism to be used with AWS OTA concept: it provides
job dispatch and status tracking over MQTT without requiring AWS code signing. The
device already has a coreMQTT MQTT connection for telemetry and can reuse it for job
notifications. Firmware binaries are stored in S3 and downloaded directly via HTTPS
presigned URL.

The CC35X1 supports OTA for three independent component pairs, each with a
primary and secondary (target) slot:

| Component    | Slot 1 ID | Slot 2 ID | Flash Size (IS25WJ064F, 8 MB) |
|--------------|-----------|-----------|-------------------------------|
| BL2          | 0         | 1         | 408 KB each (0x00066000)      |
| Wireless_FW  | 2         | 3         | 456 KB each (0x00072000)      |
| Vendor_Image | 4         | 5         | ~2.63 MB each (0x002A2000)    |

**A single OTA job can update any subset of 1–3 components** (or all three at
once). All component types use the same PSA FWU API — no special per-component
code paths are needed in the application layer. BL2 and Wireless_FW secondary
slots are always provisioned by the bootloader in flash; Vendor_Image_Slot_2
requires `Mem_cfg.ota = true` in SysConfig — already enabled (see Step 1).

---

### Architecture

```
AWS Console
  └─ Create IoT Job (job doc: {components: [{type, slot1_id, slot2_id, url, version, size}, ...]})
        │
        │  MQTT $aws/things/{name}/jobs/notify-next
        ▼
Device (existing MQTT connection)
  ├─ Receive job notification → parse components[] array
  │
  ├─ For each component in job (1–3 entries):
  │    ├─ OTA_FWU_selectTargetSlot(slot1_id, slot2_id, &targetSlot)
  │    ├─ OTA_FWU_prepareSlot(targetSlot)         → READY
  │    ├─ ota_https_download() → stream → PSA FWU write
  │    └─ psa_fwu_finish(targetSlot)               → CANDIDATE
  │
  ├─ psa_fwu_install()    (stages all CANDIDATEs at once)  → STAGED
  ├─ Update job status → SUCCEEDED
  └─ psa_fwu_request_reboot()

On next boot:
  Bootloader validates all STAGED slots → each valid component → TRIAL
  App: WiFi connects (self-test) → psa_fwu_accept() → all TRIALs → UPDATED
       (or psa_fwu_reject() → rollback all, reboot to previous firmware)
```

---

### Overview

OTA updates flow through these stages:
1. **S3 Bucket Setup** — Store firmware binaries securely
2. **Firmware Upload** — Place binary in S3
3. **Pre-signed URL Generation** — Create temporary access link
4. **Job Creation** — Define update task with firmware details
5. **Job Deployment** — Push to device via MQTT
6. **Device Download** — Device fetches firmware over HTTPS
7. **Installation** — PSA FWU writes to secondary slot
8. **Reboot & Verification** — Device switches to new firmware

---

### AWS Console Setup

#### Create an S3 Bucket

**Purpose:** Store firmware binaries and job documents securely.

**Steps:**

1. Go to **Amazon S3 Console** (https://s3.console.aws.amazon.com)
2. Click **Create bucket**
3. **Bucket name:** Enter a unique name (e.g., `osprey-ota-firmware`)
   - Must be globally unique across all AWS accounts
   - Use lowercase letters, numbers, hyphens only
   - No underscores or dots
4. **Region:** Select same region as IoT Core (e.g., `us-east-1`)
5. **Block Public Access:** Keep all checkboxes **checked** (don't make public)
6. Click **Create bucket**

**Verify bucket was created:**
- You should see it listed in the S3 console
- Note the bucket name for later steps

**Optional: Bucket Versioning (Recommended)**

For safety, enable versioning to keep old firmware versions:

1. Click your bucket name
2. Go to **Properties** tab
3. Scroll to **Versioning**
4. Click **Edit**
5. Select **Enable versioning**
6. Click **Save changes**

This allows you to recover previous firmware versions if needed.

---

### Upload a Firmware Binary

#### Prepare the Binaries

Build the firmware in CCS (with OTA enabled in SysConfig). Upon a successful compilation, a .out file is created under `Debug/<app_name>.out`. The output signed binary that needs to be eventually uploaded to AWS S3, is created under `Debug/toolbox/primary_vendor_image.sign.bin` — this file already has
the 48-byte TI PSA FWU manifest prepended by the build system. The version for the vendor application can be modified in the \*.syscfg under the **Actions Requested** option, in **Primary Vendor Image Version**.

Recommended file naming convention:

| Component | Filename pattern | Source |
|-----------|-----------------|--------|
| Vendor_Image | `vendor_vMAJOR.MINOR.REVISION.BUILD.bin` | CCS Debug output |
| BL2 | `bl2_vMAJOR.MINOR.PATCH.bin` | TI SDK build |
| Wireless_FW | `wsoc_fw_vMAJOR.MINOR.PATCH.bin` | TI SDK build |

#### Upload to S3

**Steps:**

1. Go to your S3 bucket (S3 Console → Buckets → your bucket name)
2. Click **Upload**
3. **Add files:** Click **Add files** and select your firmware binary
   - Example: `vendor_image_aws_0.1.0.1.sign.bin` (your signed binary)
4. **Storage class:** Leave as default (Standard)
5. **Permissions:** Leave as default (private)
6. Click **Upload**

**Verify upload:**
- After upload completes, you should see the file listed in the bucket
- Click the file to view details (object URL, size, last modified date)
- **Copy the object URL** for generating the pre-signed URL later

**File Naming Best Practice:**

Use semantic versioning in filename:
```
vendor_image_{project}_{version}_{timestamp}.bin
vendor_image_aws_0.1.0.1.bin          ← Good
vendor_image_aws_0.1.0.1.sign.bin     ← Good (if signed on your side)
vendor.bin                            ← Too generic
```

**File Size Considerations:**

- Device must have enough secondary flash slot for firmware
- Larger files require more download time

---

### Generate a Presigned URL for the Binary

**Purpose:** Create a temporary, secure HTTPS link that the device can use to download the firmware without AWS credentials.

#### Via S3 Console

1. Go to your S3 bucket
2. Click the firmware file you uploaded
3. Look for **"Share"** or **"Generate presigned URL"** button/link
   - (Location varies by AWS Console version)
4. **Expiration time:** Set to appropriate duration
   - Recommended: 24 hours (`86400` seconds) for testing
   - Use 1-7 days for production depending on deployment window
5. Click **Generate presigned URL**
6. **Copy the full URL** (includes all `X-Amz-*` parameters)

**Example presigned URL:**
```
https://osprey-ota-firmware.s3.us-east-1.amazonaws.com/vendor_image_aws_0.1.0.1.sign.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAIOSFODNN7EXAMPLE%2F20260524%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20260524T120621Z&X-Amz-Expires=86400&X-Amz-SignedHeaders=host&X-Amz-Signature=abcd1234...
```

**URL Structure:**
- Base: `https://{bucket}.s3.{region}.amazonaws.com/{file}`
- Query string: Authentication parameters (Algorithm, Credential, Date, Expires, Signature)
- **Important:** Full URL must be copied including all query parameters

#### Via AWS CLI (if available)

```bash
aws s3 presign s3://osprey-ota-firmware/vendor_image_aws_0.1.0.1.sign.bin \
  --expires-in 86400 \
  --region us-east-1
```

**Expiration Considerations:**

| Duration | Use Case | Example |
|----------|----------|---------|
| 1 hour (3600s) | Immediate deployment, single device | Quick testing |
| 24 hours (86400s) | Multi-device rollout in one day | Typical deployment |
| 7 days (604800s) | Staggered rollout, retry tolerance | Production deployment |

**Important:** URL becomes invalid after expiration time. Plan accordingly or regenerate as needed.

---

### Create an IoT Job

#### Create Job Document (JSON)

The job document defines what firmware to install. Create a JSON file with this structure:

**File: `job-document.json`**

```json
{
  "components": [
    {
      "type": "Vendor_Image",
      "slot1_id": 4,
      "slot2_id": 5,
      "version": "0.1.0.1",
      "url": "https://osprey-ota-firmware.s3.us-east-1.amazonaws.com/vendor_image_aws_0.1.0.1.sign.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=...",
      "size": 1220692
    }
  ]
}
```

**Field Descriptions:**

| Field | Description | Example |
|-------|-------------|---------|
| `type` | Firmware component type | `"Vendor_Image"` (also: `"BL2"`, `"Wireless_FW"`) |
| `slot1_id` | Primary flash slot ID | `4` (slot IDs: 0, 2, 4 for primary; 1, 3, 5 for secondary) |
| `slot2_id` | Secondary flash slot ID (target for update) | `5` |
| `version` | Firmware version string | `"0.1.0.1"` (format: `MAJOR.MINOR.PATCH.BUILD`) |
| `url` | **Full** pre-signed HTTPS URL to firmware | Entire URL with `?X-Amz-*` parameters |
| `size` | File size in bytes | `1220692` |

**Getting File Size:**

```bash
# Linux/macOS
ls -la vendor_image_aws_0.1.0.1.sign.bin
# Output: ... 1220692 May 24 12:06 vendor_image_aws_0.1.0.1.sign.bin

# Windows
dir vendor_image_aws_0.1.0.1.sign.bin
# Output shows file size in bytes
```

**Important Notes:**

- **URL must be complete** including all `?X-Amz-*` query parameters
- **URL buffer in device code is 2560 bytes** to accommodate long pre-signed URLs
- **Do NOT include `execution` wrapper** — AWS adds that automatically
- **Slot IDs are fixed** for your hardware (check PSA FWU configuration)

#### Upload Job Document to S3

1. Go to S3 bucket
2. Click **Upload**
3. Select `job-document.json` (the file you created above)
4. Click **Upload**

**Verify:** File appears in bucket listing

---

#### Deploy Job via AWS IoT Console

**Steps:**

1. Go to **AWS IoT Console** (https://console.aws.amazon.com/iot)
2. Left sidebar → **Manage** → **Jobs**
3. Click **Create job**
4. Choose **"Create a custom job"** (not "Create FreeRTOS OTA update job")
   - FreeRTOS option requires code signing and file paths (not applicable for embedded)

**Configure Job:**

5. **Job ID:** Enter a descriptive name
   - Example: `ota-vendor-v0.1.0.1-20260524`
   - Pattern: `ota-{component}-v{version}-{date}`

6. **Job document:**
   - Select **"Upload a custom job document"**
   - Click **"Browse S3"** or **"Upload a new file"**
   - Navigate to your `job-document.json` in S3 bucket
   - Click **Select**

7. **Job targets:**
   - Click **"Add targets"**
   - Select **"Things"**
   - Check your device Thing name (e.g., `DevMac_AABBCCDDEEFF`)
   - Click **"Add targets"**

8. **Job type:** Leave as default
   - For OTA: Typically **"Snapshot"** or **"Continuous"**
   - Recommended: **"Snapshot"** (one-time job)

9. **Advanced options:** Leave defaults
   - Rollout rate, abort config, timeout settings
   - Can be customized for large deployments

10. Click **"Create"** or **"Create and deploy"**

**Monitor Job Deployment:**

After job creation:
1. Job appears in the Jobs list
2. Click job name to see details
3. **Job execution details** tab shows target device(s) and status:
   - `QUEUED` — Device hasn't received job yet
   - `IN_PROGRESS` — Device is downloading/installing
   - `SUCCEEDED` — Installation complete, reboot pending
   - `FAILED` — Error occurred

**Check Device Execution Status:**

In the job details, click **"Job executions"** tab to see per-device status:
- Device name
- Status (QUEUED, IN_PROGRESS, SUCCEEDED, FAILED)
- Timestamps (received, started, finished)
- Error details (if failed)

**Monitor Device Logs:**

In device serial output, you should see:
```
[OTA] Job notification received (XXX bytes)
[OTA] Job parsed: 1 component(s) pending
[OTA-HTTPS] Starting download from: https://...
[OTA-HTTPS] TLS connection established
[OTA-HTTPS] HTTP response status: 200
[OTA-HTTPS] Downloaded 1220692 / 1220692 bytes
[OTA] Job {jobId} status: IN_PROGRESS
[OTA] Component installed. Rebooting...
[OTA] Job {jobId} status: SUCCEEDED
```

---

#### Troubleshooting Job Deployment

| Issue | Cause | Solution |
|-------|-------|----------|
| **Job stuck in QUEUED** | Device not receiving job | Check MQTT connection, subscription to job topics |
| **HTTP 403 Forbidden** | Pre-signed URL expired or invalid | Regenerate new presigned URL, update job document |
| **HTTP 404 Not Found** | File doesn't exist in S3 | Verify file uploaded to bucket, check filename in URL |
| **Device parse error** | Job document format incorrect | Verify JSON structure, no `execution` wrapper |
| **FWU write failed** | Secondary slot corrupted or no space | Check slot state, verify file size fits |
| **Connection timeout** | Network unstable during download | Increase timeout, check Wi-Fi signal |

---

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| **Connection refused (port 8883)** | No internet, firewall blocking | Check Wi-Fi, verify endpoint domain |
| **Certificate error (0x7280)** | Invalid cert or mTLS handshake failed | Verify cert is activated, check root CA |
| **Authentication failed** | Policy too restrictive or cert not attached | Verify policy and certificate attachment |
| **MQTT timeout** | Network delay or firewall blocking port 8883 | Use `mosquitto_pub` to test connectivity |
| **Device doesn't receive shadow updates** | Not subscribed to delta topic | Call `AwsIotLed_Subscribe()` |


---

## References

- [AWS IoT Core Documentation](https://docs.aws.amazon.com/iot-core/)
- [Device Shadow Service](https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-service.html)
- [coreMQTT Documentation](https://github.com/FreeRTOS/coreMQTT)
- [mbedTLS Documentation](https://mbed-tls.readthedocs.io/)
- [CC3551 Technical Reference Manual](https://www.ti.com/lit/pdf/swru615)

---

**Document Version:** 1.0  
**Last Updated:** May 2026
