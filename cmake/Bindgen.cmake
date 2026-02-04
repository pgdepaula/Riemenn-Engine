# FindBindgen.cmake - Helper para localizar e usar bindgen
#
# Uso:
# include(Bindgen)
# bhs_add_bindings(
#     TARGET target_name
#     WRAPPER path/to/wrapper.h
#     OUTPUT path/to/output.rs
#     INCLUDES include1 include2 ...
# )

find_program(BINDGEN_EXECUTABLE bindgen)

if(BINDGEN_EXECUTABLE)
    message(STATUS "Bindgen encontrado: ${BINDGEN_EXECUTABLE}")
else()
    message(WARNING "Bindgen não encontrado! Bindings Rust não serão atualizados.")
endif()

function(bhs_add_bindings)
    cmake_parse_arguments(ARG "" "TARGET;WRAPPER;OUTPUT" "INCLUDES" ${ARGN})

    if(NOT BINDGEN_EXECUTABLE)
        return()
    endif()

    # Prepara a lista de includes
    set(INCLUDE_FLAGS "")
    foreach(INC ${ARG_INCLUDES})
        list(APPEND INCLUDE_FLAGS "-I${INC}")
    endforeach()

    add_custom_command(
        OUTPUT "${ARG_OUTPUT}"
        COMMAND "${BINDGEN_EXECUTABLE}" "${ARG_WRAPPER}"
                --use-core
                --ctypes-prefix core::ffi
                --no-layout-tests
                --allowlist-function "bhs_.*"
                --allowlist-type "bhs_.*"
                --allowlist-var "BHS_.*"
                --output "${ARG_OUTPUT}"
                -- 
                ${INCLUDE_FLAGS}
                "-I${CMAKE_SOURCE_DIR}" # Raiz do projeto (para includes relativos como "math/vec4.h")
        DEPENDS "${ARG_WRAPPER}"
        COMMENT "Gerando bindings Rust: ${ARG_OUTPUT}"
        VERBATIM
    )

    add_custom_target(${ARG_TARGET} DEPENDS "${ARG_OUTPUT}")
endfunction()
