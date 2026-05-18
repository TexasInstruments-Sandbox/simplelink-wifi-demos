# AWS IoT Plugin Architecture for CC3551 (SimpleLink Wi-Fi Demos)

## Overview

This document describes the **AWS IoT plugin architecture** for the CC3551 microcontroller within the **SimpleLink Wi-Fi Demos** project. The plugin provides a production-ready AWS IoT Core integration with support for:

- **X.509 Mutual TLS Authentication** — Certificate-based secure connection
- **MQTT Telemetry** — Periodic sensor data publishing
- **Device Shadow** — Remote device state synchronization and control
- **coreMQTT Library** — FreeRTOS-compatible MQTT protocol stack

---

## Architecture Overview

### High-Level Component Stack

```
┌──────────────────────────────────────────────────────────────┐
│  Application Layer (User Code)                               │
│  ├─ AwsIotTelemetry_Connect/Run/Disconnect                   │
│  ├─ AwsIotLed_Subscribe/OnMqttPublish                         │
│  └─ Custom MQTT operations                                   │
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  AWS IoT Cloud Abstraction Layer                             │
│  Location: src/freertos/cloud/aws/                           │
│  ├─ aws_iot_telemetry.{c,h}    (Telemetry & subscription)   │
│  ├─ aws_iot_led.{c,h}          (Shadow-based LED control)   │
│  └─ aws_iot_mqtt.h             (Public API)                  │
└──────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│  coreMQTT Adaptation Layer (AWS-specific)                    │
│  Location: src/freertos/transport/                           │
│  ├─ aws_iot_core_mqtt.c        (MQTT wrapper for AWS)       │
│  └─ AwsIoTMQTT_Init()           (Initialize with QoS tracking)
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
2. Go to **Certificates** (left sidebar)
3. Click **Create certificate**
4. Select **Create certificate** (AWS will auto-generate)
5. Download files:
   - `certificate.pem`
   - `private.key`
   - `public.key`
   - `AmazonRootCA1.pem` (or copy from [AWS Trust Services](https://www.amazontrust.com/repository/AmazonRootCA1.pem))
6. Click **Activate** to enable the certificate
7. **Save the Certificate ARN** (e.g., `arn:aws:iot:us-east-1:123456789012:cert/abcd1234...`)

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

**Typical OTA Flow:**

1. Device receives notification on `notify-next`
2. Device requests job details via `$next/get`
3. AWS responds on `$next/get/accepted` with firmware URL and metadata
4. Device downloads firmware from pre-signed S3 URL (separate HTTPS connection)
5. Device writes firmware to secondary flash slot (PSA FWU)
6. Device publishes job status update to `jobs/{jobId}/update`
7. AWS confirms on `jobs/{jobId}/update/accepted`
8. Device reboots with new firmware

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

## Compilation & Deployment

### Building the AWS Plugin

```bash
# Build dependencies (root directory)
make build-all-ticlang

# Build example project (if available)
make example EXAMPLE=projects/LP_EM_CC35X1/aws-iot-telemetry TOOLCHAIN=ticlang
```

### Loading Certificates onto Device

Certificates must be provisioned into the CC3551's non-volatile memory (NVOCMP):

1. **During First Boot:**
   - Device runs provisioning mode (BLE + Wi-Fi)
   - Receives certificate, key, and thing name over secure channel
   - Stores in NVOCMP

2. **Via JTAG/Debug Probe:**
   - Flash pre-provisioned firmware with certs embedded
   - Use Code Composer Studio debugger

3. **Via AWS IoT Fleet Provisioning (Future):**
   - Device obtains temporary credentials
   - Registers itself with AWS
   - Receives permanent certificate

---

## Testing the Connection

### Option 1: AWS IoT Console Test Client (Easiest)

**No prerequisites — everything in the AWS Console:**

1. Go to [AWS IoT Console](https://console.aws.amazon.com/iot/home)
2. Left sidebar → **Test** (under Manage)
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

**Advantages:**
- No external tools needed
- Real-time message viewing
- No certificate file management
- Visual shadow editor

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

1. **Quick shadow control** → Use AWS Console Test or AWS CLI
2. **Monitor telemetry** → Use AWS Console Test (subscribe to `AwsTI/DevMac_*/telemetry`)
3. **Advanced MQTT testing** → Use mosquitto_pub/mosquitto_sub

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

### 3. Message Routing & Analytics

**Manage → Message Routing → Rules:**
- Create rules to forward messages to S3, DynamoDB, Lambda, etc.
- Example: Store telemetry in DynamoDB for analytics

### 4. Fleet Provisioning

**Onboard → Provision template:**
- Enable bulk device provisioning without pre-loading certificates
- Devices register themselves with AWS

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

## Next Steps

1. **Set up AWS IoT Core** following the steps in Section 2
2. **Provision device certificate** onto CC3551
3. **Compile and flash** the AWS example code
4. **Monitor telemetry** in AWS IoT Console Test tab
5. **Control LED** via Device Shadow updates

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
