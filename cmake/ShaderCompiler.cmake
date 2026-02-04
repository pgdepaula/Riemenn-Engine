# cmake/ShaderCompiler.cmake
# Compila Shaders/Kernels (OpenCL C) para SPIR-V usando Clang

function(target_compile_shaders target_name)
    # Tenta encontrar glslc (do Vulkan SDK) primeiro, pois é o padrão para GLSL
    find_program(GLSLC_EXECUTABLE NAMES glslc)
    
    # Fallback para Clang se glslc não existir (mas Clang para GLSL é menos robusto que glslc)
    # Tenta achar qualquer clang
    find_program(CLANG_EXECUTABLE NAMES clang clang-15 clang-14 clang-12)
    
    if(NOT GLSLC_EXECUTABLE AND NOT CLANG_EXECUTABLE)
         message(WARNING "Nem glslc nem clang encontrados. Shaders podem não compilar.")
    endif()

    set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/assets/shaders")
    set(SHADER_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders")

    # Encontrar arquivos de shader (.c = OpenCL kernel, .comp = GLSL Compute)
    file(GLOB SHADER_SOURCES_CL "${SHADER_SRC_DIR}/*.c")
    file(GLOB SHADER_SOURCES_GLSL "${SHADER_SRC_DIR}/*.comp")
    file(GLOB SHADER_SOURCES_VERT "${SHADER_SRC_DIR}/*.vert")
    file(GLOB SHADER_SOURCES_FRAG "${SHADER_SRC_DIR}/*.frag")
    
    # UI Shaders from gui
    set(GUI_SHADER_DIR "${CMAKE_SOURCE_DIR}/gui/ui/render/shaders")
    file(GLOB UI_SHADERS_VERT "${GUI_SHADER_DIR}/*.vert")
    file(GLOB UI_SHADERS_FRAG "${GUI_SHADER_DIR}/*.frag")

    set(SHADER_SOURCES 
        ${SHADER_SOURCES_CL} 
        ${SHADER_SOURCES_GLSL} 
        ${SHADER_SOURCES_VERT} 
        ${SHADER_SOURCES_FRAG} 
        ${UI_SHADERS_VERT} 
        ${UI_SHADERS_FRAG}
    )

    if(NOT SHADER_SOURCES)
        message(WARNING "Nenhum shader encontrado em ${SHADER_SRC_DIR}")
        return()
    endif()

    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    set(SPV_FILES "")

    # Flags para Clang (OpenCL)
    set(CLANG_FLAGS
        -c
        -target spirv64
        -x cl
        -cl-std=CL2.0
        -Xclang -finclude-default-header
        -O3
        -I ${CMAKE_SOURCE_DIR}
        -D BHS_SHADER_COMPILER
        -D BHS_USE_FLOAT
        -Wno-unused-value
    )

    foreach(SRC ${SHADER_SOURCES})
        get_filename_component(FILENAME ${SRC} NAME_WE)
        get_filename_component(EXT ${SRC} EXT)
        
        # Preserva extensão original no nome do output para evitar colisão?
        # Por enquanto mantemos padrão: nome.spv (ou nome.ext.spv se preferir)
        # Vamos usar: nome.spv para manter compatibilidade com loader atual que espera isso
        # Decide nome de saída
        if("${EXT}" STREQUAL ".vert" OR "${EXT}" STREQUAL ".frag")
             set(SPV_OUT "${SHADER_OUT_DIR}/${FILENAME}${EXT}.spv")
        else()
             set(SPV_OUT "${SHADER_OUT_DIR}/${FILENAME}.spv")
        endif()

        if("${EXT}" STREQUAL ".comp")
            # GLSL Compute Shader
            if(GLSLC_EXECUTABLE)
                add_custom_command(
                    OUTPUT ${SPV_OUT}
                    COMMAND ${GLSLC_EXECUTABLE} -O -fshader-stage=compute ${SRC} -o ${SPV_OUT}
                    DEPENDS ${SRC}
                    COMMENT "Compilando Shader GLSL (glslc): ${FILENAME}${EXT} -> ${FILENAME}.spv"
                    VERBATIM
                )
            elseif(CLANG_EXECUTABLE)
                # Clang compiling GLSL -> SPIR-V (Experimental/Complex)
                message(FATAL_ERROR "glslc não encontrado! Instale o Vulkan SDK para compilar ${SRC}")
            endif()
        elseif("${EXT}" STREQUAL ".vert" OR "${EXT}" STREQUAL ".frag")
             # GLSL Graphics Shaders (Vertex/Fragment)
             if(GLSLC_EXECUTABLE)
                # Determine stage explicitly or let glslc deduce from extension
                add_custom_command(
                    OUTPUT ${SPV_OUT}
                    COMMAND ${GLSLC_EXECUTABLE} -O ${SRC} -o ${SPV_OUT}
                    DEPENDS ${SRC}
                    COMMENT "Compilando UI Shader: ${FILENAME}${EXT} -> ${FILENAME}.spv"
                    VERBATIM
                )
             else()
                 message(FATAL_ERROR "glslc necessário para compilar UI shaders: ${SRC}")
             endif()
        else()
            # OpenCL C Kernel (Legacy/Grid)
            if(CLANG_EXECUTABLE)
                add_custom_command(
                    OUTPUT ${SPV_OUT}
                    COMMAND ${CLANG_EXECUTABLE} ${CLANG_FLAGS} ${SRC} -o ${SPV_OUT}
                    DEPENDS ${SRC}
                    COMMENT "Compilando Kernel OpenCL: ${FILENAME}${EXT} -> ${FILENAME}.spv"
                    VERBATIM
                )
            else()
                 # Se não tem clang, avisa mas não mata o build se tivermos glslc para os outros
                 message(WARNING "Clang não encontrado. Pulando compilação de kernel OpenCL: ${SRC}")
            endif()
        endif()

        list(APPEND SPV_FILES ${SPV_OUT})
    endforeach()

    # Adicionar target customizado para garantir que shaders sejam compilados
    add_custom_target(${target_name}_shaders ALL DEPENDS ${SPV_FILES})
    
    # Adicionar dependência ao target principal
    add_dependencies(${target_name} ${target_name}_shaders)

endfunction()
