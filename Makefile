# Makefile - Wrapper do CMake para Black Hole Simulator
#
# Este arquivo makefile não faz nada além de delegar a compilação propriamente dita ao cmake.
#

# Configurações
MAKEFLAGS += --no-print-directory
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin

# Detectar número de núcleos para build paralelo
# (Tenta nproc, sysctl ou fallback para 4)
NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ==============================================================================
# Targets
# ==============================================================================

.PHONY: all auto clean help
.PHONY: linux-x11 linux-wayland windows mac rust-binds

# Se rodar 'make' puro, faz tudo automático no sistema detectado pelo cmake 
all: auto

help:
	@echo "--- Black Hole Simulator Wrapper ---"
	@echo "Targets:"
	@echo "  make              : Auto-detecta, configura e compila"
	@echo "  make clean        : Remove pasta build"
	@echo ""
	@echo "Overrides Manuais (Configura + Compila):"
	@echo "  make linux-x11"
	@echo "  make linux-wayland"
	@echo "  make windows"
	@echo "  make mac"
	@echo "  make rust-binds   : Atualiza bindings Rust (via CMake)"

# ------------------------------------------------------------------------------
# Lógica de Build
# ------------------------------------------------------------------------------

# Macro para execução unificada
# $1: Argumentos extras para o CMake (Ex: -DPLATFORM=...)
# $2: Mensagem amigável de "Modo"
define run_pipeline
	@echo "================================================================="
	@echo " [WRAPPER] $(2)"
	@echo "================================================================="
	@mkdir -p $(BUILD_DIR)
	@echo " [1/2] Configurando (CMake)..."
	@cmake -S . -B $(BUILD_DIR) $(1) $(CMAKE_ARGS)
	@echo ""
	@echo " [2/2] Compilando (Binario)..."
	@cmake --build $(BUILD_DIR) --parallel $(NPROCS)
	@echo ""
	@echo "================================================================="
	@echo " [WRAPPER] Sucesso! Executavel gerado em: $(BIN_DIR)"
	@echo "================================================================="
endef

# --- Automático ---
auto:
	$(call run_pipeline,,Modo Auto-Deteccao)

# --- Overrides Manuais ---
linux-x11:
	$(call run_pipeline,-DPLATFORM=LINUX_X11 -DGPU_API=vulkan,Forcando LINUX_X11 + Vulkan)

linux-wayland:
	$(call run_pipeline,-DPLATFORM=LINUX_WAYLAND -DGPU_API=vulkan,Forcando LINUX_WAYLAND + Vulkan)

windows:
	$(call run_pipeline,-DPLATFORM=WINDOWS -DGPU_API=dx11,Forcando WINDOWS + DX11)

mac:
	$(call run_pipeline,-DPLATFORM=MACOS -DGPU_API=metal,Forcando MACOS + Metal)

mac_x86:
	$(call run_pipeline,-DPLATFORM=MACOS_X86 -DGPU_API=metal,Forcando MACOS_X86 + Metal)

fbsd-x11:
	$(call run_pipeline,-DPLATFORM=FBSD_X11 -DGPU_API=vulkan,Forcando FBSD_X11 + Vulkan)

fbsd-wayland:
	$(call run_pipeline,-DPLATFORM=FBSD_WAYLAND -DGPU_API=vulkan,Forcando FBSD_WAYLAND + Vulkan)

navigator:
	$(call run_pipeline,-DPLATFORM=NAVIGATOR -DGPU_API=webgpu,Forcando NAVIGATOR + WebGPU)

# --- Tools ---
rust-binds:
	@cmake --build $(BUILD_DIR) --target rust-binds



# --- Limpeza ---
clean:
	@rm -rf $(BUILD_DIR)
	@echo " [WRAPPER] Build limpo."
