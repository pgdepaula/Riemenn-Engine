# ==============================================================================
# Riemenn Engine — Render Hardware Interface (RHI)
# All platform-supported backends are compiled together into the executable/engine
# to enable runtime API switching (Vulkan <-> D3D12 <-> D3D11 <-> OpenGL).
# ==============================================================================

set(RHI_SOURCES "")
set(RHI_LIBS "")

# --- Vulkan Backend (Linux & Windows) -----------------------------------------
find_package(Vulkan)
if(Vulkan_FOUND)
    file(GLOB_RECURSE VULKAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/*.c")
    list(APPEND RHI_SOURCES ${VULKAN_SOURCES})
    list(APPEND RHI_LIBS Vulkan::Vulkan)
    message(STATUS "[RHI] Vulkan backend compiled")
endif()

# --- OpenGL Backend (Linux & Windows) -----------------------------------------
file(GLOB_RECURSE OPENGL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/opengl/*.c")
if(OPENGL_SOURCES)
    list(APPEND RHI_SOURCES ${OPENGL_SOURCES})
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
        find_package(OpenGL)
        if(OpenGL_FOUND)
            list(APPEND RHI_LIBS OpenGL::GL)
        endif()
    else()
        list(APPEND RHI_LIBS opengl32)
    endif()
    message(STATUS "[RHI] OpenGL backend compiled")
endif()

# --- Windows Backends (Direct3D 12 & Direct3D 11) -----------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Direct3D 12
    file(GLOB_RECURSE DX12_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/dx/*.cpp")
    if(DX12_SOURCES)
        list(APPEND RHI_SOURCES ${DX12_SOURCES})
        list(APPEND RHI_LIBS d3d12 dxgi)
        message(STATUS "[RHI] Direct3D 12 backend compiled")
    endif()

    # Direct3D 11
    file(GLOB_RECURSE DX11_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/dx11/*.cpp")
    if(DX11_SOURCES)
        list(APPEND RHI_SOURCES ${DX11_SOURCES})
        list(APPEND RHI_LIBS d3d11 dxgi)
        message(STATUS "[RHI] Direct3D 11 backend compiled")
    endif()
endif()

# --- Library target -----------------------------------------------------------
add_library(ri_rhi STATIC ${RHI_SOURCES})
add_library(Riemenn::RHI ALIAS ri_rhi)

target_include_directories(ri_rhi PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_rhi PUBLIC ri_foundation ri_platform)
if(RHI_LIBS)
    target_link_libraries(ri_rhi PRIVATE ${RHI_LIBS})
endif()
