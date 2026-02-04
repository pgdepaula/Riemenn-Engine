# cmake/CompilerWarnings.cmake
# Configura flags de warning agressivas mas sãs para garantir qualidade de código.

function(set_project_warnings target_name)
    option(WARNINGS_AS_ERRORS "Tratar warnings como erros" ON)

    set(COMMON_WARNINGS
        -Wall
        -Wextra
        -Wshadow
        -Wunused
        -Wpedantic
        -Wformat=2
    )

    set(CXX_WARNINGS
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
    )

    if(WARNINGS_AS_ERRORS)
        list(APPEND COMMON_WARNINGS -Werror)
    endif()

    target_compile_options(${target_name} PRIVATE 
        ${COMMON_WARNINGS}
        $<$<COMPILE_LANGUAGE:CXX>:${CXX_WARNINGS}>
    )

endfunction()
