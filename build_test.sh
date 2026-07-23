#!/bin/bash

# Makefile build test for SimpleLink Wi-Fi Demos.
# CCS projectspec builds are run via Claude Code using the CCS MCP server.

set +e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMPORTS_MAK="${PROJECT_ROOT}/imports.mak"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}SimpleLink Wi-Fi Demos Makefile Build Test${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

if [ ! -f "${IMPORTS_MAK}" ]; then
    echo -e "${RED}ERROR: imports.mak not found at ${IMPORTS_MAK}${NC}"
    exit 1
fi

TICLANG_ARMCOMPILER=$(grep "^TICLANG_ARMCOMPILER" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)
GCC_ARMCOMPILER=$(grep "^GCC_ARMCOMPILER" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)

TICLANG_AVAILABLE=false
GCC_AVAILABLE=false
[ -d "${TICLANG_ARMCOMPILER}" ] && TICLANG_AVAILABLE=true
[ -d "${GCC_ARMCOMPILER}" ]     && GCC_AVAILABLE=true

echo -e "${BLUE}Toolchains:${NC}"
[ "${TICLANG_AVAILABLE}" = "true" ] \
    && echo -e "  ${GREEN}✓ TI Clang: ${TICLANG_ARMCOMPILER}${NC}" \
    || echo -e "  ${RED}✗ TI Clang NOT found: ${TICLANG_ARMCOMPILER}${NC}"
[ "${GCC_AVAILABLE}" = "true" ] \
    && echo -e "  ${GREEN}✓ GCC ARM:  ${GCC_ARMCOMPILER}${NC}" \
    || echo -e "  ${YELLOW}⚠ GCC ARM NOT found: ${GCC_ARMCOMPILER}${NC}"
echo ""

TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Remove every artifact produced by make and the SysConfig/toolbox run.
# Called both before (clean slate) and after (no leftovers) each build.
cleanup_build_dir() {
    local dir="$1"
    local mf="$2"
    (cd "${dir}" && make -f "${mf}" clean 2>/dev/null || true)
    rm -f \
        "${dir}/action_params.json" \
        "${dir}/action_request_extra.txt" \
        "${dir}/cc35xx-conf.ini" \
        "${dir}/cflags.rsp" \
        "${dir}/external_memory_configurator.json" \
        "${dir}/link.rsp" \
        "${dir}/mem_cfg_extra.txt" \
        "${dir}/syscfg_c.rov.xs" \
        "${dir}/ti_build_linker.lds.toolbox" \
        "${dir}/ti_build_linker.cmd.toolbox" \
        "${dir}/ti_utils_build_linker.cmd.genmap" \
        "${dir}/build.log"
}

run_makefile_test() {
    local build_name="$1"
    local build_dir="$2"
    local makefile="$3"
    local binary_name="$4"
    local toolchain_available="$5"

    echo -e "${BLUE}--- ${build_name} ---${NC}"

    if [ "${toolchain_available}" != "true" ]; then
        echo -e "${YELLOW}SKIPPED: toolchain not available${NC}"
        echo ""
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    if [ ! -f "${build_dir}/${makefile}" ]; then
        echo -e "${RED}FAILED: makefile not found at ${build_dir}/${makefile}${NC}"
        echo ""
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    cleanup_build_dir "${build_dir}" "${makefile}"

    (cd "${build_dir}" && make -f "${makefile}" 2>&1 | tee build.log)
    local build_status=${PIPESTATUS[0]}
    echo ""

    if [ ${build_status} -eq 0 ] && [ -f "${build_dir}/${binary_name}" ]; then
        local size
        size=$(ls -lh "${build_dir}/${binary_name}" | awk '{print $5}')
        echo -e "${GREEN}✓ PASSED — ${binary_name} (${size})${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAILED${NC}"
        echo "Last 20 lines of build output:"
        tail -20 "${build_dir}/build.log"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi

    cleanup_build_dir "${build_dir}" "${makefile}"
    echo ""
}

# ── AWS IoT MQTT ──────────────────────────────────────────────────────────────

run_makefile_test \
    "AWS IoT MQTT - TI Clang" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/aws-iot-mqtt/freertos/ticlang" \
    "makefile" \
    "aws_iot_client.out" \
    "${TICLANG_AVAILABLE}"

run_makefile_test \
    "AWS IoT MQTT - GCC" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/aws-iot-mqtt/freertos/gcc" \
    "makefile" \
    "aws_iot_client.out" \
    "${GCC_AVAILABLE}"

# ── Azure IoT MQTT ────────────────────────────────────────────────────────────

run_makefile_test \
    "Azure IoT MQTT - TI Clang" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang" \
    "makefile" \
    "azure_iot_client.out" \
    "${TICLANG_AVAILABLE}"

run_makefile_test \
    "Azure IoT MQTT - GCC" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/gcc" \
    "makefile" \
    "azure_iot_client.out" \
    "${GCC_AVAILABLE}"

# ── Summary ───────────────────────────────────────────────────────────────────

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Passed:  ${TESTS_PASSED}${NC}"
echo -e "${RED}Failed:  ${TESTS_FAILED}${NC}"
echo -e "${YELLOW}Skipped: ${TESTS_SKIPPED}${NC}"
echo ""

if [ ${TESTS_FAILED} -eq 0 ]; then
    echo -e "${GREEN}All available builds passed!${NC}"
    exit 0
else
    echo -e "${RED}Some builds failed!${NC}"
    exit 1
fi
