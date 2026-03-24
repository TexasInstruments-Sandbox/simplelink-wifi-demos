# ==============================================================================
# Azure IoT MQTT Client - GCC Makefile
# ==============================================================================

# Path to SimpleLink SDK (relative to this makefile)
SIMPLELINK_SDK_INSTALL_DIR ?= $(abspath ../../../../../resources/simplelink-wifi-sdk)
SIMPLELINK_SDK_DEMO_ROOT ?= $(abspath ../../../../..)

include $(SIMPLELINK_SDK_DEMO_ROOT)/imports.mak

CC = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
LNK = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"

# SysConfig configuration
SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler gcc --product $(SIMPLELINK_SDK_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler gcc --product $(SIMPLELINK_SDK_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output . ../aziot_client_app.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

# Application source files
APP_OBJECTS = \
    aziot_client_app.obj \
    backoff_algorithm.obj \
    syscalls_stubs.obj

# Utilities
UTIL_OBJECTS = \
    azure_sample_crypto_mbedtls.obj

# Platform abstraction (from src/freertos)
PLATFORM_OBJECTS = \
    button_if.obj \
    led_if.obj \
    crc.obj \
    nvocmp_cc35xx.obj \
    dns_if.obj \
    tcpip_if.obj \
    wifi_if.obj

# Adaptation layer
ADAPTATION_OBJECTS = \
    osi_dpl.obj \
    osi_filesystem.obj \
    uart_term.obj \
    date_time_service.obj \
    sntp_wrapper.obj \
    sntp_task.obj \
    syslog.obj

# FreeRTOS
FREERTOS_OBJECTS = \
    main_freertos.obj

# Azure SDK for C - Core
AZURE_SDK_CORE_OBJECTS = \
    az_base64.obj \
    az_context.obj \
    az_http_pipeline.obj \
    az_http_policy.obj \
    az_http_policy_logging.obj \
    az_http_policy_retry.obj \
    az_http_request.obj \
    az_http_response.obj \
    az_json_reader.obj \
    az_json_token.obj \
    az_json_writer.obj \
    az_log.obj \
    az_precondition.obj \
    az_span.obj

# Azure SDK for C - IoT
AZURE_SDK_IOT_OBJECTS = \
    az_iot_adu_client.obj \
    az_iot_common.obj \
    az_iot_hub_client.obj \
    az_iot_hub_client_c2d.obj \
    az_iot_hub_client_commands.obj \
    az_iot_hub_client_methods.obj \
    az_iot_hub_client_properties.obj \
    az_iot_hub_client_sas.obj \
    az_iot_hub_client_telemetry.obj \
    az_iot_hub_client_twin.obj \
    az_iot_provisioning_client.obj \
    az_iot_provisioning_client_sas.obj

# Azure SDK for C - Platform
AZURE_SDK_PLATFORM_OBJECTS = \
    az_nohttp.obj \
    az_posix.obj

# Azure IoT Middleware
AZURE_MIDDLEWARE_OBJECTS = \
    azure_iot.obj \
    azure_iot_adu_client.obj \
    azure_iot_hub_client.obj \
    azure_iot_hub_client_properties.obj \
    azure_iot_json_reader.obj \
    azure_iot_json_writer.obj \
    azure_iot_message.obj \
    azure_iot_provisioning_client.obj \
    azure_iot_core_mqtt.obj \
    azure_iot_jws_mbedtls.obj

# IoT Middleware Samples
IOT_SAMPLES_OBJECTS = \
    sample_azure_iot.obj

# coreMQTT
COREMQTT_OBJECTS = \
    core_mqtt.obj \
    core_mqtt_serializer.obj \
    core_mqtt_state.obj

# Transport
TRANSPORT_OBJECTS = \
    transport_socket.obj \
    transport_tls_socket_using_mbedtls.obj \
    sockets_wrapper_lwip.obj \
    mbedtls_freertos_port.obj \
    threading_alt.obj

# All objects
OBJECTS = $(APP_OBJECTS) $(UTIL_OBJECTS) $(PLATFORM_OBJECTS) $(ADAPTATION_OBJECTS) \
          $(FREERTOS_OBJECTS) $(AZURE_SDK_CORE_OBJECTS) $(AZURE_SDK_IOT_OBJECTS) \
          $(AZURE_SDK_PLATFORM_OBJECTS) $(AZURE_MIDDLEWARE_OBJECTS) $(IOT_SAMPLES_OBJECTS) \
          $(COREMQTT_OBJECTS) $(TRANSPORT_OBJECTS) $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES)))

NAME = azure_iot_client

CFLAGS += -I../.. \
    -I. \
    -I.. \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    -include FreeRTOSConfigCustom.h \
    -O3 \
    -DCC35XX \
    -DRAM_CONTAINER \
    -DSNTP_SUPPORT \
    -DNVOCMP_POSIX_MUTEX \
    -D_REENT_SMALL \
    -DMBEDTLS_CONFIG_FILE=\"config-hsm.h\" \
    -DMBEDTLS_PSA_CRYPTO_CONFIG_FILE=\"config-psa-crypto-hsm.h\" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/kernel/freertos" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/posix/gcc" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/freertos/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/freertos/portable/GCC/ARM_CM33_NTZ/non_secure" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/ble_interface/inc_adapt" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/ble_interface/inc_common" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_platform/cc35xx/inc_common" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/wifi_stack/inc_common" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_host_driver/inc_common" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_host_driver/inc_adapt" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/tools/TI_Containers" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/lwip/lwip-stack/src/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/lwip/ti_config/lwip-port/osi/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/lwip/lwip-contrib" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/ti_config/nimble-port/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/ti_config/nimble-port/porting/npl/osi/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/host/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/host/services/gap/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/host/services/gatt/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/host/services/dis/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/host/util/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/nimble/transport/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/nimble-src/porting/nimble/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/tests/include" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/ti/configs" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/ti/port" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/include/psa" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/configs" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/include/mbedtls" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedTLS/library" \
    "-I../../ti_modified_headers" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/common/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/logging" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/interface" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/ports/coreMQTT" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/interface" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/iot-middleware-freertos-samples/demos/common/connection" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/iot-middleware-freertos-samples/demos/common/utilities" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/iot-middleware-freertos-samples/demos/common/transport" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/configs" \
    "-I../../configs" \
    -mcpu=cortex-m33 \
    -mthumb \
    -std=c99 \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -ffunction-sections \
    -fdata-sections \
    -g \
    -gstrict-dwarf \
    -Wall \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include/newlib-nano" \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include"

LFLAGS += -Wl,-T,../gcc/linker_GNU.lds \
    "-Wl,-Map,$(NAME).map" \
    -mcpu=cortex-m33 \
    "-L$(SIMPLELINK_SDK_INSTALL_DIR)/source" \
    -fno-lto \
    -Wl,--start-group \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/lwip/lib/gcc/lwip.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/hostap/lib/gcc/hostap.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/lib/gcc/nimble.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/wifi_stack/lib/gcc/wifi_stack.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/ble_interface/lib/gcc/ble_interface.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_host_driver/lib/gcc/wifi_host_driver.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_platform/cc35xx/lib/gcc/wifi_platform_cc35xx.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/utils/FWU/lib/gcc/FWU.a" \
    "$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedtls/ti/lib/gcc/m33f/mbedtls.a" \
    -Wl,--end-group \
    ti_utils_build_linker.cmd.genlibs \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -nostartfiles \
    -static \
    -Wl,--gc-sections \
    -lgcc \
    -lc \
    -lm \
    --specs=nano.specs

all: postbuild

.PHONY: postbuild
postbuild: $(NAME).out
	@echo "Build complete: $(NAME).out"

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo "SysConfig generation complete"

syscfg: ../aziot_client_app.syscfg
	@ echo "Generating configuration files..."
	$(V) $(SYSCFG_CMD_STUB) --output $(@D) $<

# Application files
aziot_client_app.obj: ../../aziot_client_app.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

backoff_algorithm.obj: ../../backoff_algorithm.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

syscalls_stubs.obj: ../../syscalls_stubs.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Utilities
azure_sample_crypto_mbedtls.obj: ../../utilities/azure_sample_crypto_mbedtls.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Platform files (from src/freertos)
button_if.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/button_if.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

led_if.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/led_if.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

crc.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/crc.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

nvocmp_cc35xx.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/nvocmp_cc35xx.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

dns_if.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/dns_if.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

tcpip_if.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/tcpip_if.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

wifi_if.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/wifi_if.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Adaptation layer
osi_dpl.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/osi_dpl.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

osi_filesystem.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/osi_filesystem.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

uart_term.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/uart_term.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

date_time_service.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/date_time_service.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

sntp_wrapper.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/sntp_wrapper.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

sntp_task.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/common/sntp_task.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

syslog.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/LP_EM_CC35X1/syslog.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# FreeRTOS
main_freertos.obj: ../main_freertos.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Azure SDK for C - Core
az_base64.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_base64.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_context.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_context.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_pipeline.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_pipeline.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_policy.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_policy.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_policy_logging.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_policy_logging.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_policy_retry.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_policy_retry.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_request.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_request.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_http_response.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_http_response.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_json_reader.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_json_reader.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_json_token.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_json_token.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_json_writer.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_json_writer.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_log.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_log.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_precondition.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_precondition.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_span.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/core/az_span.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Azure SDK for C - IoT
az_iot_adu_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_adu_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_common.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_common.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_c2d.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_c2d.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_commands.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_commands.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_methods.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_methods.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_properties.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_properties.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_sas.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_sas.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_telemetry.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_telemetry.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_hub_client_twin.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_twin.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_provisioning_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_provisioning_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_iot_provisioning_client_sas.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/iot/az_iot_provisioning_client_sas.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Azure SDK for C - Platform
az_nohttp.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/platform/az_nohttp.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

az_posix.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-sdk-for-c/sdk/src/azure/platform/az_posix.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Azure IoT Middleware
azure_iot.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_adu_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_adu_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_hub_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_hub_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_hub_client_properties.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_hub_client_properties.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_json_reader.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_json_reader.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_json_writer.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_json_writer.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_message.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_message.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_provisioning_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/source/azure_iot_provisioning_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_core_mqtt.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/azure_iot_core_mqtt.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

azure_iot_jws_mbedtls.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/azure-iot-middleware-freertos/ports/mbedTLS/azure_iot_jws_mbedtls.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# IoT Middleware Samples
sample_azure_iot.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/iot-middleware-freertos-samples/demos/sample_azure_iot/sample_azure_iot.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# coreMQTT
core_mqtt.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/core_mqtt.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

core_mqtt_serializer.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/core_mqtt_serializer.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

core_mqtt_state.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/core_mqtt_state.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Transport
transport_socket.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/transport_socket.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

transport_tls_socket_using_mbedtls.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/transport_tls_socket_using_mbedtls.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

sockets_wrapper_lwip.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/sockets_wrapper_lwip.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

mbedtls_freertos_port.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/mbedtls_freertos_port.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

threading_alt.obj: $(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedtls/ti/port/threading_alt.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Build rules for syscfg-generated files
ti_drivers_config.obj: ti_drivers_config.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

ti_freertos_config.obj: ti_freertos_config.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

ti_freertos_portable_config.obj: ti_freertos_portable_config.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

ti_flash_map_config.obj: ti_flash_map_config.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Make syscfg-generated files depend on syscfg target
ti_drivers_config.c ti_freertos_config.c ti_freertos_portable_config.c ti_flash_map_config.c: syscfg

# Generic pattern rule for other .obj files
%.obj: %.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(NAME).out: $(OBJECTS)
	@ echo "Linking $@"
	$(V) $(LNK) $(OBJECTS) $(LFLAGS) -o $(NAME).out

clean:
	@ echo "Cleaning..."
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1

help:
	@echo "Azure IoT MQTT Client - Build Targets"
	@echo "======================================"
	@echo "make          - Build the example"
	@echo "make clean    - Clean build artifacts"
	@echo "make help     - Show this help message"
	@echo ""
	@echo "Set VERBOSE=1 for verbose build output"
