# Azure IoT Hub MQTT Client Example

This example demonstrates connecting an LP-EM-CC35X1 LaunchPad to Azure IoT Hub using MQTT protocol with TLS 1.2 security.

**Supported Devices:** CC3500, CC3501, CC3550, CC3551
**LaunchPad:** LP-EM-CC35X1

---

## Features

- **Azure IoT Hub connectivity** via MQTT protocol
- **Authentication methods:**
  - X.509 certificate authentication
  - Shared Access Signature (SAS) token authentication
- **TLS 1.2** with hardware-accelerated cryptography (PSA Crypto)
- **Telemetry:** Send device-to-cloud messages
- **Cloud-to-device messages:** Receive commands from Azure
- **Device Twin:** Report and receive desired properties
- **Direct Methods:** Handle cloud-invoked methods
- **FreeRTOS** task-based architecture

---

## Prerequisites

Before building this example, ensure you have:

### Hardware
- LP-EM-CC35X1 LaunchPad (with CC3500, CC3501, CC3550, or CC3551 device)
- Micro-USB cable
- Wi-Fi network with internet access

### Software
- Code Composer Studio 12.8.0+ (includes TI Clang compiler)
- **OR** ARM GCC 13.2+ (if not using TI Clang)
- SDK dependencies built (see root README.md)

### Cloud Setup
- **Azure subscription** (free tier available)
- **Azure IoT Hub** created and configured
- **Device registered** in IoT Hub with authentication credentials

---

## Building the Example

### Option 1: Command Line with Makefile

#### Build with TI Clang (default):
```bash
cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang
make
```

#### Build with GCC:
```bash
cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/gcc
make
```

#### Build from root directory:
```bash
# From repository root
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=gcc
```

### Option 2: Code Composer Studio

1. **Import project:**
   - File → Import → CCS Projects
   - Browse to: `projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/`
   - Select: `azure_client_CC35X1_LAUNCHXL_freertos_ticlang.projectspec`
   - Click Finish

2. **Build project:**
   - Right-click project → Build Project
   - Or press Ctrl+B (Windows/Linux) / Cmd+B (macOS)

### Option 3: GCC in Code Composer Studio

1. **Import project:**
   - File → Import → CCS Projects
   - Browse to: `projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/gcc/`
   - Select: `azure_client_CC35X1_LAUNCHXL_freertos_gcc.projectspec`
   - Click Finish

2. **Build project:**
   - Right-click project → Build Project

---

## Configuration

### 1. Wi-Fi Settings

Edit `wifi_settings.h` with your Wi-Fi credentials:

```c
#define WIFI_SSID             "YourNetworkName"
#define WIFI_PASSWORD         "YourPassword"
#define WIFI_SECURITY_TYPE    SL_WLAN_SEC_TYPE_WPA_WPA2
```

**Supported security types:**
- `SL_WLAN_SEC_TYPE_OPEN` - No security
- `SL_WLAN_SEC_TYPE_WPA_WPA2` - WPA/WPA2 Personal
- `SL_WLAN_SEC_TYPE_WPA2` - WPA2 Personal only
- `SL_WLAN_SEC_TYPE_WPA3` - WPA3 Personal

### 2. Azure IoT Hub Settings

Edit `mqtt_settings.h` with your Azure IoT Hub credentials:

#### Option A: SAS Token Authentication
```c
#define MQTT_BROKER_ENDPOINT  "your-hub.azure-devices.net"
#define MQTT_CLIENT_ID        "your-device-id"
#define MQTT_USERNAME         "your-hub.azure-devices.net/your-device-id/?api-version=2021-04-12"
#define MQTT_PASSWORD         "SharedAccessSignature sr=your-hub.azure-devices.net..."
```

#### Option B: X.509 Certificate Authentication
```c
#define MQTT_BROKER_ENDPOINT  "your-hub.azure-devices.net"
#define MQTT_CLIENT_ID        "your-device-id"
#define MQTT_USERNAME         "your-hub.azure-devices.net/your-device-id/?api-version=2021-04-12"
#define MQTT_PASSWORD         ""  // Empty for X.509

// Configure certificate paths in aziot_client_app.c
```

---

## Running the Example

### 1. Flash the Application

#### Using Code Composer Studio:
1. Connect LaunchPad via USB
2. Right-click project → Debug As → CCS Application
3. Click Resume (F8) to run

#### Using command line (UniFlash):
```bash
# Use TI UniFlash or loadti script
```

### 2. Monitor Serial Output

Connect a serial terminal with these settings:
- **Baud rate:** 115200
- **Data bits:** 8
- **Parity:** None
- **Stop bits:** 1
- **Flow control:** None

**Expected output:**
```
[INFO] Starting Azure IoT MQTT Client
[INFO] Connecting to Wi-Fi: YourNetworkName
[INFO] Wi-Fi connected, IP: 192.168.1.100
[INFO] Connecting to Azure IoT Hub...
[INFO] Connected to Azure IoT Hub
[INFO] Sending telemetry...
[INFO] Telemetry sent: {"temperature":25.5,"humidity":60}
```

---

## Azure IoT Hub Setup

### Create IoT Hub and Device

```bash
# Login to Azure
az login

# Create resource group
az group create --name MyResourceGroup --location westus

# Create IoT Hub (Free tier)
az iot hub create --resource-group MyResourceGroup --name MyIoTHub --sku F1

# Create device with SAS authentication
az iot hub device-identity create --hub-name MyIoTHub --device-id MyCC35xxDevice

# Get device connection string
az iot hub device-identity connection-string show --hub-name MyIoTHub --device-id MyCC35xxDevice
```

### Create Device with X.509 Authentication

```bash
# Generate self-signed certificate
openssl req -newkey rsa:2048 -nodes -keyout device.key -x509 -days 365 -out device.crt

# Create device with X.509 authentication
az iot hub device-identity create --hub-name MyIoTHub --device-id MyCC35xxDevice --auth-method x509_thumbprint --primary-thumbprint <cert-thumbprint> --secondary-thumbprint <cert-thumbprint>
```

### Extract Connection Information

From the connection string:
```
HostName=MyIoTHub.azure-devices.net;DeviceId=MyCC35xxDevice;SharedAccessKey=...
```

Map to `mqtt_settings.h`:
- `MQTT_BROKER_ENDPOINT` = `MyIoTHub.azure-devices.net`
- `MQTT_CLIENT_ID` = `MyCC35xxDevice`
- `MQTT_USERNAME` = `MyIoTHub.azure-devices.net/MyCC35xxDevice/?api-version=2021-04-12`
- `MQTT_PASSWORD` = Generate SAS token from SharedAccessKey

---

## Application Structure

```
azure-iot-mqtt/
├── aziot_client_app.c         # Main application logic
├── aziot_client_app.h         # Application header
├── wifi_settings.h            # Wi-Fi credentials (user edits)
├── mqtt_settings.h            # Azure IoT credentials (user edits)
├── components/                # Azure IoT libraries (TI-modified)
│   ├── azure-sdk-for-c/       # Azure SDK for Embedded C
│   ├── azure-iot-middleware-freertos/  # Azure IoT middleware
│   ├── coreMQTT/              # MQTT protocol library
│   └── iot-middleware-freertos-samples/  # Sample code
├── configs/                   # Configuration headers
├── freertos/                  # FreeRTOS-specific files
│   ├── ticlang/               # TI Clang build files
│   ├── gcc/                   # GCC build files
│   └── aziot_client_app.syscfg  # SysConfig configuration
└── README.md                  # This file
```

---

## Modifying the Example

### Change Telemetry Data

Edit `aziot_client_app.c`, function `send_telemetry_message()`:

```c
// Customize telemetry payload
snprintf(telemetry_payload, sizeof(telemetry_payload),
    "{\"temperature\":%.2f,\"humidity\":%d,\"custom_field\":\"%s\"}",
    temperature, humidity, custom_data);
```

### Handle Cloud-to-Device Messages

Implement callback in `aziot_client_app.c`:

```c
static void cloud_message_callback(az_iot_hub_client_c2d_request* request,
                                     void* context)
{
    // Process received message
    printf("Received C2D message: %.*s\n",
           (int)request->payload_length, request->payload);
}
```

### Handle Direct Methods

Implement method handler in `aziot_client_app.c`:

```c
static az_result handle_direct_method(az_iot_hub_client_method_request* request,
                                        az_iot_hub_client_method_response* response,
                                        void* context)
{
    if (az_span_is_content_equal(request->name, AZ_SPAN_LITERAL_FROM_STR("reboot")))
    {
        // Handle reboot command
        response->status = 200;
        return AZ_OK;
    }

    response->status = 404;  // Method not found
    return AZ_OK;
}
```

---

## Compiler Versions

### TI Clang (TI ARM Clang Compiler)
- **Minimum version:** 4.0.0.LTS
- **Recommended:** Latest LTS version from CCS
- **Download:** Bundled with Code Composer Studio

### GCC (ARM GNU Toolchain)
- **Minimum version:** 13.2
- **Recommended:** 13.2.1 or later
- **Download:** [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

**Note:** Ensure `GCC_ARMCOMPILER` in `imports.mak` points to the `bin` directory of your GCC installation.

---

## Troubleshooting

### Wi-Fi Connection Issues

**Problem:** Wi-Fi won't connect
- Verify SSID and password in `wifi_settings.h`
- Check security type matches your router
- Ensure 2.4 GHz Wi-Fi is enabled (CC35xx does not support 5 GHz)

### Azure Connection Issues

**Problem:** "MQTT connection failed"
- Verify IoT Hub endpoint in `mqtt_settings.h`
- Check device is registered in IoT Hub
- Ensure SAS token is not expired (regenerate if needed)
- Verify firewall allows outbound port 8883

**Problem:** "TLS handshake failed"
- Check system time is set correctly (required for certificate validation)
- Verify CA certificate is correctly configured
- Ensure mbedTLS is built with PSA Crypto support

### Build Issues

**Problem:** "undefined reference to `mbedtls_*`"
- Rebuild mbedTLS: `cd ../../../../../.. && make clean-mbedtls && make build-mbedtls-ticlang`

**Problem:** "cannot find azure_iot.h"
- Ensure components are linked correctly in projectspec
- Verify submodules are initialized: `git submodule status`

---

## Performance and Memory

### Typical Resource Usage
- **Flash:** ~450 KB (application code + libraries)
- **RAM:** ~80 KB (FreeRTOS heap + stacks + buffers)
- **Stack sizes:**
  - Main task: 4096 bytes
  - Network task: 3072 bytes
  - MQTT task: 4096 bytes

### Optimization Tips
- Reduce telemetry frequency to save power
- Adjust FreeRTOS heap size in `FreeRTOSConfig.h`
- Use deep sleep between telemetry sends (advanced)

---

## Additional Resources

### Azure IoT Documentation
- [Azure IoT Hub Overview](https://docs.microsoft.com/en-us/azure/iot-hub/)
- [MQTT Support in IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support)
- [X.509 Certificate Authentication](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-x509ca-overview)
- [Device Twin Documentation](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-device-twins)

### TI SimpleLink Documentation
- [CC3500 Product Page](https://www.ti.com/product/CC3500)
- [CC3501 Product Page](https://www.ti.com/product/CC3501)
- [CC3550 Product Page](https://www.ti.com/product/CC3550)
- [CC3551 Product Page](https://www.ti.com/product/CC3551)
- [LP-EM-CC35X1 LaunchPad User Guide](https://www.ti.com/lit/pdf/swru615)
- [SimpleLink SDK API Reference](https://dev.ti.com/tirex/)

### Community Support
- [TI E2E Forums - Wi-Fi](https://e2e.ti.com/support/wireless-connectivity/wi-fi-group)
- [Azure IoT Developer Forum](https://docs.microsoft.com/en-us/answers/topics/azure-iot-hub.html)

---

## License

This example is provided under the BSD-3-Clause license. See the root repository LICENSE file for details.

Third-party components (Azure IoT libraries, FreeRTOS, mbedTLS) are licensed under their respective licenses (MIT, Apache 2.0).

---

**Need help?** Open an issue on [GitHub](https://github.com/TexasInstruments/simplelink_wi-fi_demos/issues) or post on [TI E2E Forums](https://e2e.ti.com/).
