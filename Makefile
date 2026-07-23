# ==============================================================================
# SimpleLink Wi-Fi Demos - Root Makefile
# ==============================================================================
# This Makefile builds all required SDK dependencies and libraries
# ==============================================================================

# Import tool paths and configuration
include imports.mak

# Export compiler paths as environment variables (required by SDK CMake)
export TICLANG_ARMCOMPILER
export GCC_ARMCOMPILER
export IAR_ARMCOMPILER
export CMAKE
export PYTHON

# ==============================================================================
# Configuration
# ==============================================================================

SDK_DIR = resources/simplelink-wifi-sdk
MBEDTLS_TICLANG_DIR = $(SDK_DIR)/source/third_party/mbedtls/ti/lib/ticlang/m33f
MBEDTLS_GCC_DIR = $(SDK_DIR)/source/third_party/mbedtls/ti/lib/gcc/m33f

# Default toolchain for examples
TOOLCHAIN ?= ticlang

# Color output (Linux/macOS only, harmless on Windows)
COLOR_RESET = \033[0m
COLOR_BOLD = \033[1m
COLOR_GREEN = \033[32m
COLOR_BLUE = \033[34m
COLOR_YELLOW = \033[33m

# ==============================================================================
# Build Targets
# ==============================================================================

.PHONY: all help clean clean-sdk clean-mbedtls
.PHONY: build-sdk build-sdk-ticlang build-sdk-gcc
.PHONY: build-mbedtls build-mbedtls-ticlang build-mbedtls-gcc
.PHONY: check-tools

# Default target: build everything for TI Clang
all: check-tools build-all-ticlang

# Build all components for TI Clang toolchain
build-all-ticlang: build-sdk-ticlang build-mbedtls-ticlang
	@echo ""
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)✓ Build complete: All dependencies built successfully$(COLOR_RESET)"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Import project in CCS: projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/"
	@echo "  2. Or build via CLI: cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang && make"
	@echo ""

# Build all components for GCC toolchain
build-all-gcc: check-tools-gcc build-sdk-gcc build-mbedtls-gcc
	@echo ""
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)✓ Build complete: All GCC dependencies built successfully$(COLOR_RESET)"
	@echo ""

# ==============================================================================
# SimpleLink SDK Build
# ==============================================================================

build-sdk: build-sdk-ticlang

build-sdk-ticlang: check-tools
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building SimpleLink Wi-Fi SDK (TI Clang)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@cd $(SDK_DIR) && $(MAKE) build-ticlang
	@echo ""
	@echo "$(COLOR_GREEN)✓ SimpleLink SDK build complete (TI Clang)$(COLOR_RESET)"

build-sdk-gcc: check-tools-gcc
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building SimpleLink Wi-Fi SDK (GCC)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@cd $(SDK_DIR) && CMAKE_POLICY_VERSION_MINIMUM=3.21 $(MAKE) build-gcc
	@echo ""
	@echo "$(COLOR_GREEN)✓ SimpleLink SDK build complete (GCC)$(COLOR_RESET)"

# ==============================================================================
# mbedTLS Library Build
# ==============================================================================

build-mbedtls: build-mbedtls-ticlang

build-mbedtls-ticlang: check-tools
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building mbedTLS Library (TI Clang)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@echo "Building for Cortex-M33F with hardware crypto acceleration..."
	@cd $(MBEDTLS_TICLANG_DIR) && $(MAKE)
	@echo ""
	@echo "$(COLOR_GREEN)✓ mbedTLS library build complete (TI Clang)$(COLOR_RESET)"
	@echo "  → $(MBEDTLS_TICLANG_DIR)/libmbedcrypto.a"
	@echo "  → $(MBEDTLS_TICLANG_DIR)/libmbedtls.a"
	@echo "  → $(MBEDTLS_TICLANG_DIR)/libmbedx509.a"

build-mbedtls-gcc: check-tools-gcc
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building mbedTLS Library (GCC)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@echo "Building for Cortex-M33F with hardware crypto acceleration..."
	@cd $(MBEDTLS_GCC_DIR) && $(MAKE)
	@echo ""
	@echo "$(COLOR_GREEN)✓ mbedTLS library build complete (GCC)$(COLOR_RESET)"
	@echo "  → $(MBEDTLS_GCC_DIR)/libmbedcrypto.a"
	@echo "  → $(MBEDTLS_GCC_DIR)/libmbedtls.a"
	@echo "  → $(MBEDTLS_GCC_DIR)/libmbedx509.a"

# ==============================================================================
# Clean Targets
# ==============================================================================

clean: clean-sdk clean-mbedtls
	@echo ""
	@echo "$(COLOR_GREEN)✓ Clean complete$(COLOR_RESET)"

clean-sdk:
	@echo ""
	@echo "$(COLOR_YELLOW)Cleaning SimpleLink SDK build artifacts...$(COLOR_RESET)"
	@cd $(SDK_DIR) && $(MAKE) clean
	@echo "$(COLOR_GREEN)✓ SDK cleaned$(COLOR_RESET)"

clean-mbedtls: clean-mbedtls-ticlang clean-mbedtls-gcc

clean-mbedtls-ticlang:
	@echo ""
	@echo "$(COLOR_YELLOW)Cleaning mbedTLS libraries (TI Clang)...$(COLOR_RESET)"
	@if [ -d "$(MBEDTLS_TICLANG_DIR)" ]; then \
		cd $(MBEDTLS_TICLANG_DIR) && $(MAKE) clean; \
	fi
	@echo "$(COLOR_GREEN)✓ mbedTLS cleaned (TI Clang)$(COLOR_RESET)"

clean-mbedtls-gcc:
	@echo ""
	@echo "$(COLOR_YELLOW)Cleaning mbedTLS libraries (GCC)...$(COLOR_RESET)"
	@if [ -d "$(MBEDTLS_GCC_DIR)" ]; then \
		cd $(MBEDTLS_GCC_DIR) && $(MAKE) clean; \
	fi
	@echo "$(COLOR_GREEN)✓ mbedTLS cleaned (GCC)$(COLOR_RESET)"

# ==============================================================================
# Tool Verification
# ==============================================================================

check-tools:
	@echo ""
	@echo "$(COLOR_BOLD)Checking required build tools...$(COLOR_RESET)"
	@echo ""
ifndef TICLANG_ARMCOMPILER
	@echo "$(COLOR_YELLOW)⚠ ERROR: TICLANG_ARMCOMPILER not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ TICLANG_ARMCOMPILER: $(TICLANG_ARMCOMPILER)"
	@if [ ! -f "$(TICLANG_ARMCOMPILER)/bin/tiarmclang" ] && [ ! -f "$(TICLANG_ARMCOMPILER)/bin/tiarmclang.exe" ]; then \
		echo "$(COLOR_YELLOW)⚠ ERROR: TI ARM Clang compiler not found at $(TICLANG_ARMCOMPILER)$(COLOR_RESET)"; \
		exit 1; \
	fi
ifndef CMAKE
	@echo "$(COLOR_YELLOW)⚠ ERROR: CMAKE not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ CMAKE: $(CMAKE)"
	@if ! command -v $(CMAKE) > /dev/null 2>&1; then \
		echo "$(COLOR_YELLOW)⚠ ERROR: CMake not found: $(CMAKE)$(COLOR_RESET)"; \
		exit 1; \
	fi
ifndef PYTHON
	@echo "$(COLOR_YELLOW)⚠ ERROR: PYTHON not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ PYTHON: $(PYTHON)"
	@if ! command -v $(PYTHON) > /dev/null 2>&1; then \
		echo "$(COLOR_YELLOW)⚠ ERROR: Python not found: $(PYTHON)$(COLOR_RESET)"; \
		exit 1; \
	fi
	@echo ""
	@echo "$(COLOR_GREEN)✓ All required tools found$(COLOR_RESET)"

check-tools-gcc:
	@echo ""
	@echo "$(COLOR_BOLD)Checking GCC build tools...$(COLOR_RESET)"
	@echo ""
ifndef GCC_ARMCOMPILER
	@echo "$(COLOR_YELLOW)⚠ ERROR: GCC_ARMCOMPILER not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ GCC_ARMCOMPILER: $(GCC_ARMCOMPILER)"
	@if [ ! -f "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc" ] && [ ! -f "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc.exe" ]; then \
		echo "$(COLOR_YELLOW)⚠ ERROR: ARM GCC compiler not found at $(GCC_ARMCOMPILER)$(COLOR_RESET)"; \
		exit 1; \
	fi
ifndef CMAKE
	@echo "$(COLOR_YELLOW)⚠ ERROR: CMAKE not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ CMAKE: $(CMAKE)"
ifndef PYTHON
	@echo "$(COLOR_YELLOW)⚠ ERROR: PYTHON not defined in imports.mak$(COLOR_RESET)"
	@exit 1
endif
	@echo "  ✓ PYTHON: $(PYTHON)"
	@echo ""
	@echo "$(COLOR_GREEN)✓ All required tools found$(COLOR_RESET)"

# ==============================================================================
# Help Target
# ==============================================================================

help:
	@echo ""
	@echo "$(COLOR_BOLD)SimpleLink Wi-Fi Demos - Build System$(COLOR_RESET)"
	@echo "========================================"
	@echo ""
	@echo "$(COLOR_BOLD)Quick Start:$(COLOR_RESET)"
	@echo "  make                    Build all dependencies for TI Clang (default)"
	@echo ""
	@echo "$(COLOR_BOLD)Build SDK Dependencies:$(COLOR_RESET)"
	@echo "  make all                Build SDK + mbedTLS for TI Clang (same as 'make')"
	@echo "  make build-all-ticlang  Build everything for TI Clang toolchain"
	@echo "  make build-all-gcc      Build everything for GCC toolchain"
	@echo ""
	@echo "  make build-sdk          Build SimpleLink SDK (TI Clang)"
	@echo "  make build-sdk-ticlang  Build SimpleLink SDK for TI Clang"
	@echo "  make build-sdk-gcc      Build SimpleLink SDK for GCC"
	@echo ""
	@echo "  make build-mbedtls          Build mbedTLS library (TI Clang)"
	@echo "  make build-mbedtls-ticlang  Build mbedTLS for TI Clang"
	@echo "  make build-mbedtls-gcc      Build mbedTLS for GCC"
	@echo ""
	@echo "$(COLOR_BOLD)Build Examples:$(COLOR_RESET)"
	@echo "  make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang"
	@echo "  make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=gcc"
	@echo ""
	@echo "$(COLOR_BOLD)Clean Targets:$(COLOR_RESET)"
	@echo "  make clean              Clean all build artifacts"
	@echo "  make clean-sdk          Clean only SDK build artifacts"
	@echo "  make clean-mbedtls      Clean only mbedTLS build artifacts"
	@echo "  make clean-example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang"
	@echo ""
	@echo "$(COLOR_BOLD)Utility Targets:$(COLOR_RESET)"
	@echo "  make status             Show build status"
	@echo "  make check-tools        Verify all build tools are installed"
	@echo "  make help               Show this help message"
	@echo ""
	@echo "$(COLOR_BOLD)Prerequisites:$(COLOR_RESET)"
	@echo "  1. Edit imports.mak and configure tool paths"
	@echo "  2. Ensure git submodules are initialized:"
	@echo "     git submodule init && git submodule update"
	@echo ""
	@echo "$(COLOR_BOLD)Build Time Estimates:$(COLOR_RESET)"
	@echo "  SimpleLink SDK:  3-5 minutes"
	@echo "  mbedTLS:         1-2 minutes"
	@echo "  Example:         30-60 seconds"
	@echo "  Total:           4-7 minutes"
	@echo ""
	@echo "$(COLOR_BOLD)Example Workflow:$(COLOR_RESET)"
	@echo "  $(COLOR_BLUE)# 1. Build SDK dependencies$(COLOR_RESET)"
	@echo "  make"
	@echo ""
	@echo "  $(COLOR_BLUE)# 2a. Build example with TI Clang$(COLOR_RESET)"
	@echo "  make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang"
	@echo ""
	@echo "  $(COLOR_BLUE)# 2b. Or build example directly$(COLOR_RESET)"
	@echo "  cd projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang && make"
	@echo ""
	@echo "  $(COLOR_BLUE)# 3. Or import in CCS:$(COLOR_RESET)"
	@echo "  File → Import → CCS Projects"
	@echo "  Browse: projects/LP_EM_CC35X1/azure-iot-mqtt/freertos/ticlang/"
	@echo ""

# ==============================================================================
# Example Build Targets
# ==============================================================================

.PHONY: example example-ticlang example-gcc clean-example

# Build example with specified toolchain
# Usage: make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang
example: example-$(TOOLCHAIN)

example-ticlang:
ifndef EXAMPLE
	@echo "$(COLOR_YELLOW)ERROR: EXAMPLE not specified$(COLOR_RESET)"
	@echo "Usage: make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang"
	@exit 1
endif
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building Example: $(EXAMPLE) (TI Clang)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@cd $(EXAMPLE)/freertos/ticlang && $(MAKE)
	@echo ""
	@echo "$(COLOR_GREEN)✓ Example build complete$(COLOR_RESET)"

example-gcc:
ifndef EXAMPLE
	@echo "$(COLOR_YELLOW)ERROR: EXAMPLE not specified$(COLOR_RESET)"
	@echo "Usage: make example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=gcc"
	@exit 1
endif
	@echo ""
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)Building Example: $(EXAMPLE) (GCC)$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)$(COLOR_BOLD)========================================$(COLOR_RESET)"
	@echo ""
	@cd $(EXAMPLE)/freertos/gcc && $(MAKE)
	@echo ""
	@echo "$(COLOR_GREEN)✓ Example build complete$(COLOR_RESET)"

clean-example:
ifndef EXAMPLE
	@echo "$(COLOR_YELLOW)ERROR: EXAMPLE not specified$(COLOR_RESET)"
	@echo "Usage: make clean-example EXAMPLE=projects/LP_EM_CC35X1/azure-iot-mqtt TOOLCHAIN=ticlang"
	@exit 1
endif
	@echo ""
	@echo "$(COLOR_YELLOW)Cleaning example: $(EXAMPLE) ($(TOOLCHAIN))$(COLOR_RESET)"
	@if [ "$(TOOLCHAIN)" = "gcc" ]; then \
		cd $(EXAMPLE)/freertos/gcc && $(MAKE) clean; \
	else \
		cd $(EXAMPLE)/freertos/ticlang && $(MAKE) clean; \
	fi
	@echo "$(COLOR_GREEN)✓ Example cleaned$(COLOR_RESET)"

# ==============================================================================
# Status Target (show what's built)
# ==============================================================================

.PHONY: status

status:
	@echo ""
	@echo "$(COLOR_BOLD)Build Status$(COLOR_RESET)"
	@echo "========================================"
	@echo ""
	@echo "$(COLOR_BOLD)SimpleLink SDK:$(COLOR_RESET)"
	@if [ -d "$(SDK_DIR)/build/ticlang" ]; then \
		echo "  $(COLOR_GREEN)✓ TI Clang build directory exists$(COLOR_RESET)"; \
	else \
		echo "  $(COLOR_YELLOW)✗ TI Clang not built$(COLOR_RESET)"; \
	fi
	@if [ -d "$(SDK_DIR)/build/gcc" ]; then \
		echo "  $(COLOR_GREEN)✓ GCC build directory exists$(COLOR_RESET)"; \
	else \
		echo "  $(COLOR_YELLOW)✗ GCC not built$(COLOR_RESET)"; \
	fi
	@echo ""
	@echo "$(COLOR_BOLD)mbedTLS Libraries:$(COLOR_RESET)"
	@if [ -f "$(MBEDTLS_TICLANG_DIR)/libmbedtls.a" ]; then \
		echo "  $(COLOR_GREEN)✓ TI Clang mbedTLS built$(COLOR_RESET)"; \
	else \
		echo "  $(COLOR_YELLOW)✗ TI Clang mbedTLS not built$(COLOR_RESET)"; \
	fi
	@if [ -f "$(MBEDTLS_GCC_DIR)/libmbedtls.a" ]; then \
		echo "  $(COLOR_GREEN)✓ GCC mbedTLS built$(COLOR_RESET)"; \
	else \
		echo "  $(COLOR_YELLOW)✗ GCC mbedTLS not built$(COLOR_RESET)"; \
	fi
	@echo ""
	@echo "$(COLOR_BOLD)Build Tool Paths:$(COLOR_RESET)"
	@echo "  TICLANG_ARMCOMPILER: $(TICLANG_ARMCOMPILER)"
	@echo "  GCC_ARMCOMPILER:     $(GCC_ARMCOMPILER)"
	@echo "  CMAKE:               $(CMAKE)"
	@echo "  PYTHON:              $(PYTHON)"
	@echo ""
