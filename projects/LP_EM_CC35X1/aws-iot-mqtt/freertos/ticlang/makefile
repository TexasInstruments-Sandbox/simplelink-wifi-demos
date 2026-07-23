# ==============================================================================
# AWS IoT MQTT Client - TI Clang Makefile
# ==============================================================================

# Path to SimpleLink SDK (relative to this makefile)
SIMPLELINK_SDK_INSTALL_DIR ?= $(abspath ../../../../../resources/simplelink-wifi-sdk)
SIMPLELINK_SDK_DEMO_ROOT ?= $(abspath ../../../../..)

include $(SIMPLELINK_SDK_DEMO_ROOT)/imports.mak

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"

# SysConfig configuration
SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang --product $(SIMPLELINK_SDK_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler ticlang --product $(SIMPLELINK_SDK_INSTALL_DIR)/.metadata/product.json --product $(SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output . ../aws_iot_client_app.syscfg)

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
    aws_iot_client_app.obj \
    syscalls_stubs.obj

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

# AWS cloud layer
CLOUD_OBJECTS = \
    aws_iot_telemetry.obj \
    aws_iot_led.obj \
    aws_iot_ota.obj \
    aws_iot_ota_https.obj

# coreMQTT
COREMQTT_OBJECTS = \
    core_mqtt.obj \
    core_mqtt_serializer.obj \
    core_mqtt_state.obj

# coreHTTP
COREHTTP_OBJECTS = \
    core_http_client.obj \
    api.obj \
    http.obj \
    llhttp.obj

# coreJSON
COREJSON_OBJECTS = \
    core_json.obj

# backoffAlgorithm
BACKOFF_OBJECTS = \
    backoff_algorithm.obj

# Transport
TRANSPORT_OBJECTS = \
    aws_iot_core_mqtt.obj \
    transport_tls_socket_using_mbedtls.obj \
    sockets_wrapper_lwip.obj \
    mbedtls_freertos_port.obj \
    threading_alt.obj

# All objects
OBJECTS = $(APP_OBJECTS) $(PLATFORM_OBJECTS) $(ADAPTATION_OBJECTS) \
          $(FREERTOS_OBJECTS) $(CLOUD_OBJECTS) $(COREMQTT_OBJECTS) \
          $(COREHTTP_OBJECTS) $(COREJSON_OBJECTS) $(BACKOFF_OBJECTS) \
          $(TRANSPORT_OBJECTS) $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES)))

NAME = aws_iot_client

CFLAGS += -I../.. \
    -I. \
    -I.. \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    -include FreeRTOSConfig.h \
    -Ofast \
    -DCC35XX \
    -DRAM_CONTAINER \
    -DSNTP_SUPPORT \
    -DNVOCMP_POSIX_MUTEX \
    -DMBEDTLS_CONFIG_FILE=\"config-hsm.h\" \
    -DMBEDTLS_PSA_CRYPTO_CONFIG_FILE=\"config-psa-crypto-hsm.h\" \
    -DHTTP_DO_NOT_USE_CUSTOM_CONFIG \
    -DdemoconfigCLIENT_CERTIFICATE_PEM=\"\" \
    -DdemoconfigCLIENT_PRIVATE_KEY_PEM=\"\" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/kernel/freertos" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/posix/ticlang" \
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
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/utils/FWU" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/utils/FWU/headers" \
    "-I$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/utils/FWU/headers/fwu/1.0/psa" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/inc" \
    "-I../../ti_modified_headers" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/ns/lwip/common/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/platform/inc" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/aws" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/cloud/aws" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/logging" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreMQTT/source/interface" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/dependency/3rdparty/llhttp/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreJSON/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/backoffAlgorithm/source/include" \
    "-I$(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/configs" \
    "-I../../configs" \
    -gdwarf-3 \
    -mcpu=cortex-m33 \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -Wall


LFLAGS += "-L$(SIMPLELINK_SDK_INSTALL_DIR)/source" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/lwip/lib/ticlang/lwip.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/hostap/lib/ticlang/hostap.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/nimble/lib/ticlang/nimble.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/wifi_stack/lib/ticlang/wifi_stack.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/net/ble_interface/lib/ticlang/ble_interface.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_host_driver/lib/ticlang/wifi_host_driver.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/wifi_platform/cc35xx/lib/ticlang/wifi_platform_cc35xx.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/ti/utils/FWU/lib/ticlang/FWU.a" \
    "-Wl,-l$(SIMPLELINK_SDK_INSTALL_DIR)/source/third_party/mbedtls/ti/lib/ticlang/m33f/mbedtls.a" \
    -Wl,--reread_libs \
    ti_utils_build_linker.cmd.genlibs \
    linker.cmd \
    "-Wl,-m,$(NAME).map" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ARMCOMPILER)/lib" \
    -llibc.a

all: postbuild

.PHONY: postbuild
postbuild: $(NAME).out
	@echo "Build complete: $(NAME).out"

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo "SysConfig generation complete"

syscfg: ../aws_iot_client_app.syscfg
	@ echo "Generating configuration files..."
	$(V) $(SYSCFG_CMD_STUB) --output $(@D) $<

# Application files
aws_iot_client_app.obj: ../../aws_iot_client_app.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

syscalls_stubs.obj: ../../syscalls_stubs.c $(SYSCFG_H_FILES)
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

# AWS cloud layer
aws_iot_telemetry.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/cloud/aws/aws_iot_telemetry.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

aws_iot_led.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/cloud/aws/aws_iot_led.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

aws_iot_ota.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/cloud/aws/aws_iot_ota.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

aws_iot_ota_https.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/cloud/aws/aws_iot_ota_https.c $(SYSCFG_H_FILES)
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

# coreHTTP
core_http_client.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/core_http_client.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

api.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/dependency/3rdparty/llhttp/src/api.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

http.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/dependency/3rdparty/llhttp/src/http.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

llhttp.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreHTTP/source/dependency/3rdparty/llhttp/src/llhttp.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# coreJSON
core_json.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/coreJSON/source/core_json.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# backoffAlgorithm
backoff_algorithm.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/resources/third-party/backoffAlgorithm/source/backoff_algorithm.c $(SYSCFG_H_FILES)
	@ echo "Building $@"
	$(V) $(CC) $(CFLAGS) -c $< -o $@

# Transport
aws_iot_core_mqtt.obj: $(SIMPLELINK_SDK_DEMO_ROOT)/src/freertos/transport/aws_iot_core_mqtt.c $(SYSCFG_H_FILES)
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
	@printf '%s ' -Wl,-u,_c_int00 $(OBJECTS) $(LFLAGS) > link.rsp
	$(V) $(LNK) @link.rsp -o $(NAME).out

clean:
	@ echo "Cleaning..."
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
	$(V) $(RM) link.rsp > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1

help:
	@echo "AWS IoT MQTT Client - Build Targets"
	@echo "===================================="
	@echo "make          - Build the example"
	@echo "make clean    - Clean build artifacts"
	@echo "make help     - Show this help message"
	@echo ""
	@echo "Set VERBOSE=1 for verbose build output"
