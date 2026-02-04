cmake_minimum_required(VERSION 3.16)

# ==============================================================================
# Forçar Clang como compilador padrão
# ==============================================================================

# C (Padrão Kernel Style)
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(CLANG_C_BIN clang)
    if(CLANG_C_BIN)
        set(CMAKE_C_COMPILER "${CLANG_C_BIN}" CACHE STRING "C compiler" FORCE)
        message(STATUS "Forçando Clang para C: ${CMAKE_C_COMPILER}")
    else()
        message(WARNING "Clang não encontrado! Usando padrão do sistema.")
    endif()
endif()

# C++ (Bosta, mas as vezes necessário)
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_CXX_BIN clang++)
    if(CLANG_CXX_BIN)
        set(CMAKE_CXX_COMPILER "${CLANG_CXX_BIN}" CACHE STRING "C++ compiler" FORCE)
        message(STATUS "Forçando Clang para C++: ${CMAKE_CXX_COMPILER}")
    else()
        message(WARNING "Clang++ não encontrado! Usando padrão do sistema.")
    endif()
endif()

# Objective-C/C++ (Apple Clang no macOS)
if(APPLE)
    if(NOT DEFINED CMAKE_OBJC_COMPILER)
        set(CMAKE_OBJC_COMPILER "${CMAKE_C_COMPILER}" CACHE STRING "ObjC compiler" FORCE)
    endif()
    if(NOT DEFINED CMAKE_OBJCXX_COMPILER)
        set(CMAKE_OBJCXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE STRING "ObjC++ compiler" FORCE)
    endif()
endif()

# Projeto: Black Hole Simulator
# Lema: "Se não roda em hardware de 2010, você está fazendo errado."
project(BlackHoleSimulator
    VERSION 0.1.0
    DESCRIPTION "Real-time Black Hole Ray Marcher"
    LANGUAGES C CXX
)

# ==============================================================================
# Configurações Globais
# ==============================================================================

# Impedir builds dentro da fonte (source code pollution)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "
    ############################################################################
    # ERRO: In-source builds não são permitidas.
    # Por favor, crie um diretório de build separado:
    #   mkdir build
    #   cd build
    #   cmake ..
    ############################################################################
    ")
endif()

# Definir C11 e C++17 como padrão
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # Necessário para "Kernel Style" (GNU extensions como ##__VA_ARGS__)

set(CMAKE_CXX_STANDARD 17) # Para códigos .cpp (ferramentas, wrappers)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Caminho para módulos CMake customizados
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Diretórios de Saída (Binários e Bibliotecas)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ==============================================================================
# Módulos Utilitários
# ==============================================================================

include(AutoDetectEnvironment) # Detecção automática (Wayland/X11/Vulkan/etc)
include(PlatformValidation)    # Validação e Warnings (Ex: Mac+OpenGL)


include(CompilerWarnings)
include(Sanitizers)
include(ShaderCompiler)
include(Bindgen)

# --- Rust Bindings ---
# Gera include/bindings.rs a partir de rust/wrapper.h
# Invocavel via 'make rust-binds' (se usar Makefile wrapper que chama cmake build)
# ou ninja rust-binds / make rust-binds (direto no build dir)
bhs_add_bindings(
    TARGET rust-binds
    WRAPPER "${CMAKE_SOURCE_DIR}/rust/wrapper.h"
    OUTPUT "${CMAKE_SOURCE_DIR}/include/bindings.rs"
    INCLUDES "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/math" "${CMAKE_SOURCE_DIR}/engine" "${CMAKE_SOURCE_DIR}/gui"
)

# Opções de Build
option(BHS_ENABLE_TESTING "Habilitar testes unitários" ON)
if(BHS_ENABLE_TESTING)
    enable_testing()
endif()
option(BHS_ENABLE_SANITIZERS "Habilitar Address/Undefined Sanitizers" OFF)

# Configurar Sanitizers se solicitado
if(BHS_ENABLE_SANITIZERS)
    enable_sanitizers(bhs_options)
endif()

# ==============================================================================
# Adicionar Subdiretórios (Camadas)
# ==============================================================================

# 1. Math (Base, zero dependencies)
add_subdirectory(math)

# 2. gui (Depende de Math)
add_subdirectory(gui)

# 3. Engine (Depende de Math, gui)
add_subdirectory(engine)

# 4. Source/App (Glue code, Executable)
add_subdirectory(src)

# 5. Lua Bindings (PUC)
# add_subdirectory(lua/puc)

# ==============================================================================
# Testing
# ==============================================================================

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "Build configurada para: ${CMAKE_SYSTEM_NAME} (${CMAKE_BUILD_TYPE})")
