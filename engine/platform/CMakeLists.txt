# ==============================================================================
# Riemenn Engine — Platform Abstraction Layer
# Supported platforms: Wayland (Linux), Win32 (Windows)
# ==============================================================================

find_package(PkgConfig REQUIRED)

set(PLATFORM_SOURCES "")
set(PLATFORM_LIBS "")

# --- Wayland backend (Linux) --------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)
    file(GLOB_RECURSE WAYLAND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/wayland/*.c")
    list(APPEND PLATFORM_SOURCES ${WAYLAND_SOURCES})
    list(APPEND PLATFORM_LIBS ${WAYLAND_LIBRARIES})
    message(STATUS "[Platform] Wayland backend enabled")
endif()

# --- Win32 backend (Windows) --------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    file(GLOB_RECURSE WIN32_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/win32/*.cpp")
    list(APPEND PLATFORM_SOURCES ${WIN32_SOURCES})
    message(STATUS "[Platform] Win32 backend enabled")
endif()

# --- Library target -----------------------------------------------------------
add_library(ri_platform STATIC ${PLATFORM_SOURCES})
add_library(Riemenn::Platform ALIAS ri_platform)

target_include_directories(ri_platform PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_platform PUBLIC ri_foundation)

if(PLATFORM_LIBS)
    target_link_libraries(ri_platform PRIVATE ${PLATFORM_LIBS})
endif()

# Wayland include dirs need to be available for protocol headers
if(WAYLAND_FOUND)
    target_include_directories(ri_platform PRIVATE ${WAYLAND_INCLUDE_DIRS})
endif()
