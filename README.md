# SimpleLink Wi-Fi Demos

Production-ready Azure IoT connectivity examples for Texas Instruments CC35xx wireless MCUs with FreeRTOS.

**Supported Hardware:** [CC3501](https://www.ti.com/tool/LP-EM-CC3501) / [CC3511](https://www.ti.com/tool/LP-EM-CC3511) LaunchPads

---

## Overview

This repository demonstrates:
- Azure IoT Hub connectivity via MQTT
- FreeRTOS integration with SimpleLink SDK
- mbedTLS with PSA Crypto hardware acceleration
- Secure TLS 1.2 communication (X.509 or SAS token)

---

## Repository Structure

```
simplelink_wi-fi_demos/
├── projects/LP_EM_CC35X1/        # Example applications
│   └── azure-iot-mqtt/           # Azure IoT Hub MQTT client
│       ├── components/           # TI-customized Azure IoT libraries
│       └── freertos/             # CCS project files (ticlang/gcc)
├── src/freertos/                 # Shared platform abstraction layers
├── resources/
│   ├── simplelink-wifi-sdk/      # TI SDK (submodule)
│   └── third-party/              # Upstream Azure IoT libraries (submodules)
└── imports.mak                   # Build tool paths configuration
```

**Key Distinction:**
- `resources/third-party/` = Original upstream libraries (reference only)
- `projects/.../components/` = TI-modified versions (used in builds)

---

## Prerequisites

Install these tools before building:

| Tool | Version | Purpose | Download |
|------|---------|---------|----------|
| **Code Composer Studio** | 12.8.0+ | IDE, compiler, debugger | [ti.com/tool/CCSTUDIO](https://www.ti.com/tool/CCSTUDIO) |
| **SysConfig** | 1.20.0+ | Pin/peripheral configuration | Bundled with CCS |
| **Git** | 2.13+ | Submodule support | [git-scm.com](https://git-scm.com/downloads) |
| **GNU Make** | Any | Build automation | Pre-installed (Linux/macOS)<br>Windows: [GnuWin32](http://gnuwin32.sourceforge.net/packages/make.htm) |
| **CMake** | 3.21+ | SDK build system | [cmake.org](https://cmake.org/download/) |
| **Python** | 3.7+ | Build scripts | [python.org](https://www.python.org/downloads/) |

**Optional:** [ARM GCC 12.3+](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) (if not using TI Clang)

---

## Getting Started

### 1. Clone Repository

```bash
# Clone with all submodules
git clone --recurse-submodules https://github.com/TexasInstruments/simplelink_wi-fi_demos.git
cd simplelink_wi-fi_demos

# Verify submodules initialized (no - or + prefixes)
git submodule status
```

<details>
<summary>If you cloned without --recurse-submodules</summary>

```bash
git submodule init
git submodule update
```
</details>

---

### 2. Configure Build Tools

Edit `imports.mak` and set paths to your installed tools:

**Linux/macOS Example:**
```makefile
SYSCONFIG_TOOL      ?= /home/username/ti/ccs1280/ccs/utils/sysconfig_1.20.0/sysconfig_cli.sh
CMAKE               ?= /usr/local/bin/cmake
PYTHON              ?= python3
TICLANG_ARMCOMPILER ?= /home/username/ti/ccs1280/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS
```

**Windows Example:**
```makefile
SYSCONFIG_TOOL      ?= C:/ti/ccs1280/ccs/utils/sysconfig_1.20.0/sysconfig_cli.bat
CMAKE               ?= C:/Program Files/CMake/bin/cmake.exe
PYTHON              ?= python
TICLANG_ARMCOMPILER ?= C:/ti/ccs1280/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS
```

**Important:** Use absolute paths, no spaces, forward slashes on Windows.

---

### 3. Build Dependencies and Project

Run these commands sequentially from the repository root:

```bash
# Build SimpleLink SDK (FreeRTOS kernel, drivers, networking stack)
cd resources/simplelink-wifi-sdk && make build-ticlang

# Build mbedTLS library (crypto with hardware acceleration)
cd source/third_party/mbedtls/ti/lib/ticlang/m33f && make

# Return to repository root
cd ../../../../../../..

# Import project in CCS and build, OR build via command line:
cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang && make all
```

**Expected build time:** 3-8 minutes total

---

### 4. Configure and Run

**Edit connection settings:**
```bash
# Wi-Fi credentials
projects/LP_EM_CC35X1/azure-iot-mqtt/wifi_settings.h

# Azure IoT Hub credentials
projects/LP_EM_CC35X1/azure-iot-mqtt/mqtt_settings.h
```

**Flash to device:**
- **CCS:** Right-click project → Debug As → CCS Application → Run (F8)
- **Command-line:** Use UniFlash or `ccs_base/scripting/examples/loadti/loadti.sh`

**Serial terminal:** 115200 baud, 8N1, no flow control

---

## Quick Reference

### Building in Code Composer Studio

1. File → Import → CCS Projects
2. Browse: `projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/`
3. Select: `azure_client_CC35X1_LAUNCHXL_freertos_ticlang.projectspec`
4. Build Project (Ctrl+B / Cmd+B)

### Azure IoT Configuration

**mqtt_settings.h:**
```c
#define MQTT_BROKER_ENDPOINT  "your-hub.azure-devices.net"
#define MQTT_CLIENT_ID        "your-device-id"
#define MQTT_USERNAME         "your-hub.azure-devices.net/your-device-id/?api-version=2021-04-12"
#define MQTT_PASSWORD         "SharedAccessSignature sr=..." // Or empty for X.509
```

**wifi_settings.h:**
```c
#define WIFI_SSID             "YourNetworkName"
#define WIFI_PASSWORD         "YourPassword"
#define WIFI_SECURITY_TYPE    SL_WLAN_SEC_TYPE_WPA_WPA2
```

---

## Third-Party Components

Azure IoT libraries are tracked as git submodules for version control:

| Component | Version | Purpose |
|-----------|---------|---------|
| azure-sdk-for-c | v1.6.0-beta.1 | Azure SDK for Embedded C |
| azure-iot-middleware-freertos | v1.2.0-beta.1 | Azure IoT FreeRTOS middleware |
| coreMQTT | v2.3.1+ | FreeRTOS MQTT protocol library |
| iot-middleware-freertos-samples | main | Azure IoT reference samples |

**TI Platform Modifications** (in `projects/.../components/`):
- Increased timeouts for wireless (240s keep-alive, 20min TX/RX)
- C99 function signature compatibility
- Removed unused platform features
- Const qualifier adjustments for TI compiler

---

## Troubleshooting

### Submodule Issues

```bash
# Empty directories in resources/
git submodule init && git submodule update

# Submodule on wrong commit (+ prefix in status)
cd resources/third-party/<submodule-name>
git checkout <commit-hash>  # From 'git submodule status' expected value
```

### Build Errors

**`cannot find sys/stat.h`**
→ Fixed in latest code, update: `git pull origin master`

**`undefined reference to mbedtls_*`**
→ Rebuild: `cd resources/simplelink-wifi-sdk/source/third_party/mbedtls/ti/lib/ticlang/m33f && make clean && make`

**`SYSCONFIG_TOOL not found`**
→ Update path in `imports.mak`, find with: `which sysconfig_cli.sh`

**`command not found` during build**
→ Verify tool paths: `${TICLANG_ARMCOMPILER}/bin/tiarmclang --version`

### Runtime Issues

**Wi-Fi won't connect**
→ Check SSID/password in `wifi_settings.h`, verify security type

**Azure connection fails**
→ Verify device registered in IoT Hub, check credentials in `mqtt_settings.h`

**TLS handshake failed**
→ Check system time set correctly (required for cert validation)

**CCS import fails**
→ Build SDK first: `cd resources/simplelink-wifi-sdk && make build-ticlang`, then reimport

---

## Additional Resources

**Documentation:**
- [CC35xx Technical Reference Manual](https://www.ti.com/lit/pdf/swru615)
- [SimpleLink SDK User Guide](https://dev.ti.com/tirex/explore/node?node=A__AHCNcbE0w4VCQN4yz4hFnw__com.ti.SIMPLELINK_CC13XX_CC26XX_SDK__BSEc4rl__LATEST)
- [Azure IoT Hub Docs](https://docs.microsoft.com/en-us/azure/iot-hub/)
- [FreeRTOS Docs](https://www.freertos.org/Documentation/RTOS_book.html)

**Support:**
- [TI E2E Forums](https://e2e.ti.com/)
- [GitHub Issues](https://github.com/TexasInstruments/simplelink_wi-fi_demos/issues)

**Create Azure IoT Hub (free tier):**
```bash
az login
az group create --name MyResourceGroup --location westus
az iot hub create --resource-group MyResourceGroup --name MyIoTHub --sku F1
az iot hub device-identity create --hub-name MyIoTHub --device-id MyCC35xxDevice
az iot hub device-identity connection-string show --hub-name MyIoTHub --device-id MyCC35xxDevice
```

---

## License

- Example code: BSD-3-Clause
- SimpleLink SDK: See `resources/simplelink-wifi-sdk/LICENSE`
- Azure IoT components: MIT License
- FreeRTOS: MIT License
- mbedTLS: Apache 2.0

---

## Version History

**v1.0.0** (2026-03-05)
- Initial release with Azure IoT MQTT example
- CC3501/CC3511 LaunchPad support
- FreeRTOS + mbedTLS with PSA Crypto

---

**Questions?** Open an issue on [GitHub](https://github.com/TexasInstruments/simplelink_wi-fi_demos/issues) or visit [TI E2E Forums](https://e2e.ti.com/).
