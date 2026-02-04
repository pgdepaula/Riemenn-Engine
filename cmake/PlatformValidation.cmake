# ==============================================================================
# PlatformValidation.cmake
# ==============================================================================
#
# Valida as combinações de PLATFORM e GPU_API escolhidas (ou detectadas).
# É aqui que damos bronca no usuário se ele fizer besteira.
#
# ==============================================================================

if(PLATFORM STREQUAL "MACOS_X86" AND GPU_API STREQUAL "opengl")
    message(WARNING "
    ############################################################################
    #                         ⚠️  AVISO DE DEPRECIAÇÃO ⚠️                       #
    ############################################################################
    #                                                                          #
    #  Você escolheu OpenGL no MacOS (Intel).                                  #
    #  A Apple depreciou o OpenGL há anos. O suporte é precário e lento.       #
    #                                                                          #
    #  A menos que você tenha um motivo muito bom (ou seja masoquista),        #
    #  considere usar METAL (-DGPU_API=metal).                                 #
    #                                                                          #
    #  Prosseguindo por sua conta e risco...                                   #
    ############################################################################
    ")
endif()

if(PLATFORM STREQUAL "NAVIGATOR" AND NOT GPU_API STREQUAL "webgpu")
    message(FATAL_ERROR "Navigator (Web) suporta apenas WebGPU. Selecione -DGPU_API=webgpu.")
endif()
