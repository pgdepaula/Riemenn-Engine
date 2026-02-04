 # gui/CMakeLists.txt

add_library(bhs_gui STATIC)

find_package(PkgConfig REQUIRED)

# --- 1. Fontes Core (Log e UI genérica) ---
set(gui_SOURCES
    "log.c"
    "ui/context.c"
    "ui/layout.c"
    "ui/theme.c"
    "ui/widgets.c"
    "ui/window/window.c"
    "ui/render/render2d.c"
    "ui/render/font_system.c"
)

# --- 2. Seleção de RHI (API Gráfica) ---
message(STATUS "[GUI] Selecionando Backend RHI: ${GPU_API}")

if(GPU_API STREQUAL "vulkan")
    file(GLOB rhi_SOURCES "rhi/vulkan/*.c")
    # Remover rhi_vulkan.c se ele for realmente legado/morto
    list(FILTER rhi_SOURCES EXCLUDE REGEX "rhi_vulkan.c") 
    
    find_package(Vulkan REQUIRED)
    target_link_libraries(bhs_gui PRIVATE Vulkan::Vulkan)

    # --- Font System Dependencies ---
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    target_link_libraries(bhs_gui PRIVATE ${FONTCONFIG_LIBRARIES} ${FREETYPE_LIBRARIES})
    target_include_directories(bhs_gui PRIVATE ${FONTCONFIG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})

elseif(GPU_API STREQUAL "webgpu")
    file(GLOB rhi_SOURCES "rhi/webgpu/*.c")
    # WebGPU deps? (dawn, emscripten, etc)

elseif(GPU_API STREQUAL "opengl")
    file(GLOB rhi_SOURCES "rhi/opengl/*.c")
    find_package(OpenGL REQUIRED)
    target_link_libraries(bhs_gui PRIVATE OpenGL::GL)

elseif(GPU_API STREQUAL "dx11" OR GPU_API STREQUAL "dx")
    file(GLOB rhi_SOURCES "rhi/dx/*.c")

elseif(GPU_API STREQUAL "metal")
    file(GLOB rhi_SOURCES "rhi/metal/*.c")
endif()

list(APPEND gui_SOURCES ${rhi_SOURCES})

# --- 3. Seleção de EPA (Abstração de Plataforma) ---
message(STATUS "[GUI] Selecionando Backend EPA: ${PLATFORM}")

if(PLATFORM MATCHES "WAYLAND")
    file(GLOB epa_SOURCES "epa/wayland/*.c")
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)
    target_link_libraries(bhs_gui PRIVATE ${WAYLAND_LIBRARIES})
    target_include_directories(bhs_gui PRIVATE ${WAYLAND_INCLUDE_DIRS})

elseif(PLATFORM MATCHES "X11")
    file(GLOB epa_SOURCES "epa/x11/*.c")
    find_package(X11 REQUIRED)
    target_link_libraries(bhs_gui PRIVATE X11::X11)
    target_include_directories(bhs_gui PRIVATE ${X11_INCLUDE_DIRS})

elseif(PLATFORM STREQUAL "WINDOWS")
    file(GLOB epa_SOURCES "epa/win32/*.c")

elseif(PLATFORM STREQUAL "MACOS")
    file(GLOB epa_SOURCES "epa/cocoa/*.m" "epa/cocoa/*.c")

elseif(PLATFORM STREQUAL "NAVIGATOR")
    file(GLOB epa_SOURCES "epa/navigator/*.c")
endif()

list(APPEND gui_SOURCES ${epa_SOURCES})

target_sources(bhs_gui PRIVATE ${gui_SOURCES})

# Includes Públicos
target_include_directories(bhs_gui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(bhs_gui PUBLIC bhs_math)

add_library(BHS::gui ALIAS bhs_gui)

if(BHS_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
