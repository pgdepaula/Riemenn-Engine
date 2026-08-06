cmake_minimum_required(VERSION 3.20)
project(RiemennEngine VERSION 0.1.0 LANGUAGES C CXX)

# Require Clang
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Clang is the required C compiler for Riemenn-Engine.")
endif()

# Standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(RI_ENABLE_TESTING "Enable tests" OFF)
option(RI_ENABLE_SANITIZERS "Enable ASAN/UBSAN" OFF)

# Anti in-source build
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory.")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ShaderCompiler OPTIONAL)

if(RI_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

# Subdirectories
add_subdirectory(engine)
add_subdirectory(game)

if(RI_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
