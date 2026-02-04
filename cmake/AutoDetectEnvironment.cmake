# ==============================================================================
# AutoDetectEnvironment.cmake
# ==============================================================================
#
# Detecta automaticamente o ambiente do usuário para evitar dores de cabeça.
# Ninguém merece ficar configurando -DPLATFORM=LINUX_WAYLAND na mão.
#
# Se o usuário definir -DPLATFORM ou -DGPU_API manualmente, nós respeitamos.
# (A menos que ele esteja errado, mas aí é problema dele).
#
# ==============================================================================

function(detect_environment)
    # Variáveis temporárias
    set(_DETECTED_PLATFORM "UNKNOWN")
    set(_DETECTED_API "UNKNOWN")
    set(_NEEDS_DETECTION ON)

    # Se o usuário já definiu tudo ou se já está no cache, pulamos a detecção pesada.
    # MAS ainda queremos imprimir o resumo no final.
    if(DEFINED PLATFORM AND DEFINED GPU_API)
        set(_NEEDS_DETECTION OFF)
    endif()

    if(_NEEDS_DETECTION)
        message(STATUS "----------------------------------------------------------------")
        message(STATUS "[AUTO-DETECT] Iniciando varredura do sistema...")

        # --------------------------------------------------------------------------
        # 1. Detectar Sistema Operacional e Subsistema Gráfico
        # --------------------------------------------------------------------------
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            message(STATUS "[AUTO-DETECT] SO: Linux (O único SO que importa)")
            
            # Verificar Wayland vs X11
            if("$ENV{XDG_SESSION_TYPE}" MATCHES "wayland" OR DEFINED ENV{WAYLAND_DISPLAY})
                set(_DETECTED_PLATFORM "LINUX_WAYLAND")
                message(STATUS "[AUTO-DETECT] Display Server: Wayland (Moderno, elegante, sem tearing)")
            else()
                set(_DETECTED_PLATFORM "LINUX_X11")
                message(STATUS "[AUTO-DETECT] Display Server: X11 (Clássico, ou talvez você só não tenha Wayland)")
            endif()

        elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set(_DETECTED_PLATFORM "WINDOWS")
            message(STATUS "[AUTO-DETECT] SO: Windows (Sinto muito por você)")

        elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(_DETECTED_PLATFORM "MACOS")
            message(STATUS "[AUTO-DETECT] SO: MacOS (Prepare a carteira)")

        elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            if("$ENV{XDG_SESSION_TYPE}" MATCHES "wayland" OR DEFINED ENV{WAYLAND_DISPLAY})
                set(_DETECTED_PLATFORM "FBSD_WAYLAND")
            else()
                set(_DETECTED_PLATFORM "FBSD_X11")
            endif()
            message(STATUS "[AUTO-DETECT] SO: FreeBSD (Respect)")

        else()
            message(WARNING "[AUTO-DETECT] SO Alienígena detectado: ${CMAKE_SYSTEM_NAME}. Fallback para LINUX_X11 genérico.")
            set(_DETECTED_PLATFORM "LINUX_X11")
        endif()

        # --------------------------------------------------------------------------
        # 2. Detectar API Gráfica (GPU)
        # --------------------------------------------------------------------------
        find_package(Vulkan QUIET)

        if(_DETECTED_PLATFORM MATCHES "MACOS")
            set(_DETECTED_API "metal")
            message(STATUS "[AUTO-DETECT] GPU API: Metal (Obrigatório no Mac)")
        
        elseif(Vulkan_FOUND)
            set(_DETECTED_API "vulkan")
            message(STATUS "[AUTO-DETECT] GPU API: Vulkan (SDK encontrado em ${Vulkan_INCLUDE_DIRS})")
            message(STATUS "[AUTO-DETECT]          Este é o caminho.")

        elseif(_DETECTED_PLATFORM MATCHES "WINDOWS")
            set(_DETECTED_API "dx11")
            message(STATUS "[AUTO-DETECT] GPU API: Vulkan não achado, fallback para DX11 no Windows")

        else()
            if(_DETECTED_PLATFORM MATCHES "NAVIGATOR")
                set(_DETECTED_API "webgpu")
            else()
                set(_DETECTED_API "opengl")
                message(STATUS "[AUTO-DETECT] GPU API: Vulkan não achado. Fallback para OpenGL (Legacy).")
            endif()
        endif()

        # --------------------------------------------------------------------------
        # 3. Aplicar Definições (Se não houver override)
        # --------------------------------------------------------------------------
        
        if(NOT DEFINED PLATFORM)
            set(PLATFORM ${_DETECTED_PLATFORM} CACHE STRING "Plataforma Alvo" FORCE)
            message(STATUS "[AUTO-DETECT] PLATFORM definido para: ${PLATFORM}")
        endif()

        if(NOT DEFINED GPU_API)
            set(GPU_API ${_DETECTED_API} CACHE STRING "API Gráfica Alvo" FORCE)
            message(STATUS "[AUTO-DETECT] GPU_API definido para: ${GPU_API}")
        endif()
    endif()

    # Resumo Final para o Usuário (Sempre exibe, mesmo vindo do cache)
    message(STATUS "")
    message(STATUS "================================================================")
    message(STATUS " DEFINIÇÃO DE BUILD ")
    message(STATUS "================================================================")
    message(STATUS " PLATAFORMA   : ${PLATFORM}")
    message(STATUS " API GRÁFICA  : ${GPU_API}")
    message(STATUS "================================================================")
    message(STATUS "")

endfunction()

# Executar detecção imediatamente ao incluir
detect_environment()
