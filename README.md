# SimpleLink Wi-Fi Demos

Example applications demonstrating wireless connectivity for Texas Instruments CC35xx wireless MCUs.

**Supported Hardware:** [LP-EM-CC35X1](https://www.ti.com/tool/LP-EM-CC35X1) LaunchPad (supports CC3500, CC3501, CC3550, CC3551)

---

## Overview

This repository provides production-ready examples demonstrating:
- Cloud connectivity (Azure IoT, AWS IoT, HTTP/HTTPS clients)
- FreeRTOS integration with SimpleLink SDK
- Secure TLS communication with hardware-accelerated crypto
- Network stack configuration and optimization

---

## Repository Structure

```
simplelink_wi-fi_demos/
├── projects/
│   └── LP_EM_CC35X1/           # Examples for CC3500/CC3501/CC3550/CC3551
│       └── azure-iot-mqtt/     # Azure IoT Hub MQTT client example
├── src/freertos/               # Shared platform abstraction layers
│   ├── ns/                     # Network stack interfaces (DNS, MQTT, TCP/IP, Wi-Fi)
│   ├── platform/               # Platform-specific implementations
│   ├── transport/              # TLS transport layer
│   └── logging/                # Logging utilities
├── resources/
│   ├── simplelink-wifi-sdk/    # SimpleLink Wi-Fi SDK (git submodule)
│   └── third-party/            # Third-party libraries (git submodules)
├── imports.mak                 # Build tool paths configuration
└── Makefile                    # Root-level build system
```

**Key Concepts:**
- `src/freertos/` = Shared code used across multiple examples
- `projects/*/` = Example applications with example-specific code
- `resources/` = External dependencies tracked as git submodules

---

## Supported Devices

| Device | Part Number | Features |
|--------|-------------|----------|
| **CC3500** | CC3500MRGKT | 2.4 GHz Wi-Fi 6, Arm Cortex-M33, 2MB Flash |
| **CC3501** | CC3501MRGKT | 2.4 GHz Wi-Fi 6 + Bluetooth LE 5.4, Arm Cortex-M33, 2MB Flash |
| **CC3550** | CC3550MRGKT | 2.4 GHz + 5 GHz Wi-Fi 6, Arm Cortex-M33, 2MB Flash |
| **CC3551** | CC3551MRGKT | 2.4 GHz + 5 GHz Wi-Fi 6 + Bluetooth LE 5.4, Arm Cortex-M33, 2MB Flash |

All devices share the same LaunchPad form factor: **LP-EM-CC35X1**

---

## Prerequisites

Install these tools before building:

| Tool | Version | Purpose | Download |
|------|---------|---------|----------|
| **Code Composer Studio** | 12.8.0+ | IDE, TI Clang compiler, debugger | [ti.com/tool/CCSTUDIO](https://www.ti.com/tool/CCSTUDIO) |
| **SysConfig** | 1.20.0+ | Pin/peripheral configuration | Bundled with CCS |
| **Git** | 2.13+ | Submodule support | [git-scm.com](https://git-scm.com/downloads) |
| **GNU Make** | Any | Build automation | Pre-installed (Linux/macOS)<br>Windows: [GnuWin32](http://gnuwin32.sourceforge.net/packages/make.htm) |
| **CMake** | 3.21+ | SDK build system | [cmake.org](https://cmake.org/download/) |
| **Python** | 3.7+ | Build scripts | [python.org](https://www.python.org/downloads/) |

**Optional (for GCC builds):**
- [ARM GCC 13.2+](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) - Set `GCC_ARMCOMPILER` in `imports.mak`

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
GCC_ARMCOMPILER     ?= /usr/local/gcc-arm-none-eabi-13.2/bin
```

**Windows Example:**
```makefile
SYSCONFIG_TOOL      ?= C:/ti/ccs1280/ccs/utils/sysconfig_1.20.0/sysconfig_cli.bat
CMAKE               ?= C:/Program Files/CMake/bin/cmake.exe
PYTHON              ?= python
TICLANG_ARMCOMPILER ?= C:/ti/ccs1280/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS
GCC_ARMCOMPILER     ?= C:/gcc-arm-none-eabi-13.2/bin
```

**Important:** Use absolute paths, no spaces, forward slashes recommended.

---

### 3. Build SDK Dependencies

From the repository root, build the SimpleLink SDK libraries:

```bash
# Option 1: Build all dependencies for TI Clang (default)
make

# Option 2: Build all dependencies for GCC
make build-all-gcc

# Option 3: Build dependencies individually
make build-sdk-ticlang        # SimpleLink SDK with TI Clang
make build-mbedtls-ticlang    # mbedTLS library with TI Clang
```

**Expected build time:** 4-7 minutes total

---

### 4. Build Examples

#### Option A: Command Line (Makefile)

```bash
# Build azure-iot-mqtt example with TI Clang
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang

# Build azure-iot-mqtt example with GCC
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=gcc

# Or navigate to example directory
cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang
make
```

#### Option B: Code Composer Studio

1. File → Import → CCS Projects
2. Browse to: `projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/`
3. Select: `azure_client_CC35X1_LAUNCHXL_freertos_ticlang.projectspec`
4. Click Finish, then Build Project (Ctrl+B / Cmd+B)

---

## Examples

| Example | Description | Cloud Provider |
|---------|-------------|----------------|
| **azure-iot-mqtt** | Connect to Azure IoT Hub via MQTT with X.509 or SAS authentication | Azure |

More examples coming soon (AWS IoT, HTTP clients, OTA updates).

---

## Makefile Reference

### Root Makefile Targets

```bash
make                    # Build all dependencies for TI Clang (default)
make help               # Show all available targets

# Build Dependencies
make build-all-ticlang  # SDK + mbedTLS for TI Clang
make build-all-gcc      # SDK + mbedTLS for GCC
make build-sdk          # SimpleLink SDK only (TI Clang)
make build-mbedtls      # mbedTLS library only (TI Clang)

# Build Examples
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang
make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=gcc

# Clean
make clean              # Clean all build artifacts
make clean-sdk          # Clean SDK build artifacts
make clean-mbedtls      # Clean mbedTLS build artifacts

# Status
make status             # Show what's been built
```

### Example Makefile Targets

```bash
cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang

make                    # Build example
make clean              # Clean example build artifacts
make help               # Show example-specific targets
```

---

## Third-Party Components

All third-party libraries are tracked as git submodules:

| Component | Version | Purpose |
|-----------|---------|---------|
| **simplelink-wifi-sdk** | 9.22.00.15 | TI's SimpleLink Wi-Fi SDK |
| azure-sdk-for-c | v1.6.0-beta.1 | Azure SDK for Embedded C |
| azure-iot-middleware-freertos | v1.2.0-beta.1 | Azure IoT FreeRTOS middleware |
| coreMQTT | v2.3.1+ | FreeRTOS MQTT protocol library |
| iot-middleware-freertos-samples | main | Azure IoT reference samples |

**Note:** Some components in `projects/*/components/` contain TI-specific modifications (timeout adjustments, compiler compatibility fixes).

---

## Troubleshooting

### Submodule Issues

```bash
# Empty directories in resources/
git submodule init && git submodule update

# Submodule on wrong commit (+ prefix in git submodule status)
cd resources/third-party/<submodule-name>
git checkout <commit-hash>  # From expected commit in 'git submodule status'
```

### Build Errors

**`SYSCONFIG_TOOL not found`**
→ Update path in `imports.mak`, find with: `which sysconfig_cli.sh` (Linux/macOS)

**`TICLANG_ARMCOMPILER not defined`**
→ Set path in `imports.mak` to your TI ARM Clang installation

**`CMAKE not found`**
→ Install CMake and add to PATH, or set absolute path in `imports.mak`

**`undefined reference to mbedtls_*`**
→ Rebuild mbedTLS: `make clean-mbedtls && make build-mbedtls-ticlang`

**`error: use of undeclared identifier`**
→ Clean and rebuild: `make clean && make`

### Import Errors in CCS

**"Project import failed"**
→ Build SDK dependencies first: `make build-all-ticlang`

**"Cannot find syscfg files"**
→ Generated during first build. Build via command line first, then reimport.

---

## Additional Resources

**Documentation:**
- [CC35xx Technical Reference Manual](https://www.ti.com/lit/pdf/swru615)
- [SimpleLink CC35xx SDK User Guide](https://dev.ti.com/tirex/explore/node?node=A__AHCNcbE0w4VCQN4yz4hFnw__com.ti.SIMPLELINK_CC13XX_CC26XX_SDK__BSEc4rl__LATEST)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)

**Support:**
- [TI E2E Forums](https://e2e.ti.com/) - Community support
- [GitHub Issues](https://github.com/TexasInstruments/simplelink_wi-fi_demos/issues) - Bug reports and feature requests

**Training:**
- [SimpleLink Academy](https://dev.ti.com/tirex/explore/node?node=A__ABCX8XTjVxZ66BjJjGYucg__com.ti.SIMPLELINK_ACADEMY_CC13XX_CC26XX__AfkT0TQ__LATEST) - Interactive tutorials
- [TI Training Portal](https://training.ti.com/) - Video courses

---

## License

- Example code: BSD-3-Clause
- SimpleLink SDK: See `resources/simplelink-wifi-sdk/LICENSE`
- Azure IoT components: MIT License
- FreeRTOS: MIT License
- mbedTLS: Apache 2.0

See individual component directories for full license texts.

---

## Version History

**v1.0.0** (2025-03-13)
- Initial release
- Azure IoT MQTT example for LP-EM-CC35X1 (CC3500/CC3501/CC3550/CC3551)
- Support for TI Clang and GCC toolchains
- FreeRTOS + mbedTLS with PSA Crypto hardware acceleration

---

**Questions?** Open an issue on [GitHub](https://github.com/TexasInstruments/simplelink_wi-fi_demos/issues) or visit [TI E2E Forums](https://e2e.ti.com/).
