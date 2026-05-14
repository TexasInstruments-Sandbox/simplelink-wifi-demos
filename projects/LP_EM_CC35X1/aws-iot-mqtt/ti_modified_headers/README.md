# TI Modified Headers and Sources

This directory contains Texas Instruments' proprietary modifications to open-source Azure IoT libraries. These files contain TI-specific adaptations that are **not present in the upstream public repositories**.

## Why These Files Exist

The public versions of these libraries (from GitHub) don't match TI's SDK requirements. TI has made proprietary modifications to:
1. Match struct sizes and layouts with their coreMQTT implementation
2. Add custom transport interface features
3. Fix API compatibility issues

**Important**: These files are kept separate from the git submodules to avoid modifying third-party code in version control.

## Files in This Directory

### Headers

1. **azure_iot_mqtt.h**
   - **Modified**: Added `xheaderLength` field to `AzureIoTMQTTPacketInfo_t` struct
   - **Reason**: Match struct size with coreMQTT's `MQTTPacketInfo_t` (fixes runtime assertion failure)
   - **Original**: `resources/third-party/azure-iot-middleware-freertos/source/interface/`

2. **azure_iot_transport_interface.h**
   - **Modified**: Added `xWritev` member and `AzureIoTTransportWritev_t` function pointer
   - **Reason**: TI's transport implementation requires vectored write support
   - **Original**: `resources/third-party/azure-iot-middleware-freertos/source/interface/`

3. **core_mqtt_serializer.h**
   - **Modified**: TI-specific version with additional fields/APIs
   - **Reason**: Match TI's coreMQTT library implementation
   - **Original**: `resources/third-party/coreMQTT/source/include/`

### Note on azure_iot_core_mqtt.c

The TI-modified version of `azure_iot_core_mqtt.c` already exists in `src/freertos/transport/azure_iot_core_mqtt.c` and is used directly from there. It is not duplicated in this directory.

## How This Works

The makefiles (`freertos/ticlang/makefile` and `freertos/gcc/makefile`) are configured to:

1. **Include this directory FIRST** in the include path:
   ```makefile
   "-I../../ti_modified_headers" \
   ```
   This ensures TI-modified headers override the submodule versions.

2. **Use TI-modified sources from appropriate locations**:
   - Headers from this directory (via include path precedence)
   - `azure_iot_core_mqtt.c` from `src/freertos/transport/`
   - Other transport files from `src/freertos/transport/`

## Submodule Versions

The git submodules point to specific public repository commits:
- `azure-iot-middleware-freertos`: commit 4502a30 (v1.1.0-25)
- `coreMQTT`: (check with `git submodule status`)
- `iot-middleware-freertos-samples`: (check with `git submodule status`)

**These public versions are NOT modified**. All TI modifications are isolated in this directory.

## Version Mismatch

The runtime assertion failure you encountered:
```
Assertion failed, (sizeof(AzureIoTMQTTPacketInfo_t) == sizeof(MQTTPacketInfo_t))
```

Was caused by the public azure-iot-middleware-freertos not having the `xheaderLength` field that TI's coreMQTT expects. This is now fixed with the TI-modified `azure_iot_mqtt.h` in this directory.

## Maintenance

If you update the git submodules to newer versions, you may need to:
1. Check if TI has released updated modified versions
2. Verify struct sizes and API compatibility
3. Update files in this directory if needed
4. Re-test the build and runtime behavior

---

**DO NOT modify files in the `resources/third-party/` submodules.** All TI-specific changes should be maintained in this directory.
