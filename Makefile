# ==============================================================================
# Riemenn Engine — Developer Makefile
# Supported platforms: Linux (Wayland), Windows (Win32 + D3D12)
# ==============================================================================

.PHONY: all clean test linux windows debug release

NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_DIR = build
CMAKE     = cmake

# Default target
all: linux

# --- Linux (Wayland + Vulkan) -------------------------------------------------
linux:
	@mkdir -p $(BUILD_DIR)/linux
	cd $(BUILD_DIR)/linux && $(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR)/linux

# --- Windows (Win32 + D3D12/Vulkan) -------------------------------------------
windows:
	@mkdir -p $(BUILD_DIR)/windows
	cd $(BUILD_DIR)/windows && $(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_SYSTEM_NAME=Windows
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR)/windows

# --- Build configurations -----------------------------------------------------
debug:
	@mkdir -p $(BUILD_DIR)/debug
	cd $(BUILD_DIR)/debug && $(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR)/debug

release:
	@mkdir -p $(BUILD_DIR)/release
	cd $(BUILD_DIR)/release && $(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR)/release

# --- Tests --------------------------------------------------------------------
test:
	@mkdir -p $(BUILD_DIR)/test
	cd $(BUILD_DIR)/test && $(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DRI_ENABLE_TESTING=ON
	$(MAKE) -j$(NPROCS) -C $(BUILD_DIR)/test
	cd $(BUILD_DIR)/test && ctest --output-on-failure

# --- Cleanup ------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
