#!/bin/bash

# Build Test Script for Azure IoT MQTT Example
# Tests makefiles and CCS projectspecs using imports.mak definitions

set +e  # Don't exit on error (we handle errors explicitly)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the project root (simplelink_wi-fi_demos directory)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMPORTS_MAK="${PROJECT_ROOT}/imports.mak"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Azure IoT MQTT Build Test${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Project Root: ${PROJECT_ROOT}"
echo "Imports File: ${IMPORTS_MAK}"
echo ""

# Source the imports.mak to get toolchain paths
if [ ! -f "${IMPORTS_MAK}" ]; then
    echo -e "${RED}ERROR: imports.mak not found at ${IMPORTS_MAK}${NC}"
    exit 1
fi

# Parse imports.mak for toolchain paths
TICLANG_ARMCOMPILER=$(grep "^TICLANG_ARMCOMPILER" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)
GCC_ARMCOMPILER=$(grep "^GCC_ARMCOMPILER" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)
SYSCONFIG_TOOL=$(grep "^SYSCONFIG_TOOL" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)
CCS_INSTALL_DIR=$(grep "^CCS_INSTALL_DIR" "${IMPORTS_MAK}" | cut -d'?' -f2 | cut -d'=' -f2 | xargs)

echo -e "${BLUE}Toolchain Configuration:${NC}"
echo "  TICLANG_ARMCOMPILER: ${TICLANG_ARMCOMPILER}"
echo "  GCC_ARMCOMPILER: ${GCC_ARMCOMPILER}"
echo "  SYSCONFIG_TOOL: ${SYSCONFIG_TOOL}"
echo "  CCS_INSTALL_DIR: ${CCS_INSTALL_DIR}"
echo ""

# Check if toolchains exist
TICLANG_AVAILABLE=false
GCC_AVAILABLE=false
CCS_AVAILABLE=false

if [ -d "${TICLANG_ARMCOMPILER}" ]; then
    TICLANG_AVAILABLE=true
    echo -e "${GREEN}✓ TI Clang toolchain found${NC}"
else
    echo -e "${RED}✗ TI Clang toolchain NOT found${NC}"
fi

if [ -d "${GCC_ARMCOMPILER}" ]; then
    GCC_AVAILABLE=true
    echo -e "${GREEN}✓ GCC ARM toolchain found${NC}"
else
    echo -e "${YELLOW}⚠ GCC ARM toolchain NOT found${NC}"
fi

if [ -d "${CCS_INSTALL_DIR}" ]; then
    CCS_CLI="${CCS_INSTALL_DIR}/ccs/ccs-server.app/Contents/MacOS/ccs-server-cli.sh"
    if [ -f "${CCS_CLI}" ]; then
        CCS_AVAILABLE=true
        echo -e "${GREEN}✓ CCS installation found${NC}"
    else
        echo -e "${YELLOW}⚠ CCS directory found but ccs-server-cli.sh not found${NC}"
    fi
else
    echo -e "${YELLOW}⚠ CCS NOT found${NC}"
fi

echo ""

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Function to run a makefile build test
run_makefile_test() {
    local build_name="$1"
    local build_dir="$2"
    local makefile="$3"
    local toolchain_available="$4"

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Testing: ${build_name}${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo "Build Directory: ${build_dir}"
    echo "Makefile: ${makefile}"
    echo ""

    if [ "${toolchain_available}" != "true" ]; then
        echo -e "${YELLOW}SKIPPED: Toolchain not available${NC}"
        echo ""
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    if [ ! -f "${build_dir}/${makefile}" ]; then
        echo -e "${RED}FAILED: Makefile not found${NC}"
        echo ""
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    # Clean first
    echo "Cleaning previous build..."
    (cd "${build_dir}" && make -f "${makefile}" clean 2>&1 | tail -5) || true
    echo ""

    # Build
    echo "Building..."
    (cd "${build_dir}" && make -f "${makefile}" 2>&1 | tee build.log)
    local build_status=${PIPESTATUS[0]}

    if [ ${build_status} -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✓ BUILD PASSED${NC}"

        # Check for output binary
        if [ -f "${build_dir}/azure_iot_client.out" ]; then
            local size=$(ls -lh "${build_dir}/azure_iot_client.out" | awk '{print $5}')
            echo -e "${GREEN}  Binary created: azure_iot_client.out (${size})${NC}"
        fi

        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo ""
        echo -e "${RED}✗ BUILD FAILED${NC}"
        echo ""
        echo "Last 20 lines of build output:"
        tail -20 "${build_dir}/build.log"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

# Function to run a CCS projectspec build test
run_ccs_projectspec_test() {
    local build_name="$1"
    local projectspec_path="$2"
    local toolchain_available="$3"

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Testing: ${build_name}${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo "Projectspec: ${projectspec_path}"
    echo ""

    if [ "${CCS_AVAILABLE}" != "true" ]; then
        echo -e "${YELLOW}SKIPPED: CCS not available${NC}"
        echo ""
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    if [ "${toolchain_available}" != "true" ]; then
        echo -e "${YELLOW}SKIPPED: Toolchain not available${NC}"
        echo ""
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    if [ ! -f "${projectspec_path}" ]; then
        echo -e "${RED}FAILED: Projectspec not found${NC}"
        echo ""
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    # Create a unique temporary workspace
    local workspace="/tmp/ccs_test_$$"
    mkdir -p "${workspace}"

    echo "Importing and building project..."
    echo "Workspace: ${workspace}"
    echo ""

    # Import and build in one command using -ccs.autoBuild
    "${CCS_CLI}" \
        -workspace "${workspace}" \
        -application projectImport \
        -ccs.location "${projectspec_path}" \
        -ccs.renameTo "azure_iot_test" \
        -ccs.autoBuild \
        > "${workspace}/build.log" 2>&1

    local build_status=$?

    if [ ${build_status} -eq 0 ] && ! grep -q "Build Failed" "${workspace}/build.log"; then
        echo ""
        echo -e "${GREEN}✓ BUILD PASSED${NC}"

        # Look for the output binary
        local binary=$(find "${workspace}" -name "azure_iot_client.out" -o -name "*.out" | head -1)
        if [ -n "${binary}" ] && [ -f "${binary}" ]; then
            local size=$(ls -lh "${binary}" | awk '{print $5}')
            echo -e "${GREEN}  Binary created: $(basename ${binary}) (${size})${NC}"
        fi

        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo ""
        echo -e "${RED}✗ BUILD FAILED${NC}"
        echo ""
        echo "Last 30 lines of build output:"
        tail -30 "${workspace}/build.log"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi

    # Clean up workspace
    rm -rf "${workspace}"
    echo ""
}

# Test 1: TI Clang Makefile
run_makefile_test \
    "TI Clang Makefile" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang" \
    "makefile" \
    "${TICLANG_AVAILABLE}"

# Test 2: GCC Makefile
run_makefile_test \
    "GCC Makefile" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/gcc" \
    "makefile" \
    "${GCC_AVAILABLE}"

# Test 3: CCS TI Clang Projectspec
run_ccs_projectspec_test \
    "CCS TI Clang Projectspec" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/azure_client_CC35X1_LAUNCHXL_freertos_ticlang.projectspec" \
    "${TICLANG_AVAILABLE}"

# Test 4: CCS GCC Projectspec
run_ccs_projectspec_test \
    "CCS GCC Projectspec" \
    "${PROJECT_ROOT}/projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/gcc/azure_client_CC35X1_LAUNCHXL_freertos_gcc.projectspec" \
    "${GCC_AVAILABLE}"

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Build Test Summary${NC}"
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
