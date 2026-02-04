# cmake/Sanitizers.cmake
# Configuração fácil de Sanitizers para debug profundo.

function(enable_sanitizers project_name)

    if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
        
        set(SANITIZERS "")

        option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ON)
        option(ENABLE_SANITIZER_UNDEFINED "Enable undefined behavior sanitizer" ON)
        option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF) # Geralmente incluso no Address

        if(ENABLE_SANITIZER_THREAD AND ENABLE_SANITIZER_ADDRESS)
            message(FATAL_ERROR "ThreadSanitizer e AddressSanitizer não podem ser usados juntos.")
        endif()

        if(ENABLE_SANITIZER_ADDRESS)
            list(APPEND SANITIZERS "address")
        endif()

        if(ENABLE_SANITIZER_UNDEFINED)
            list(APPEND SANITIZERS "undefined")
        endif()

        if(ENABLE_SANITIZER_THREAD)
            list(APPEND SANITIZERS "thread")
        endif()
        
        if(ENABLE_SANITIZER_LEAK)
            list(APPEND SANITIZERS "leak")
        endif()

        list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

        if(LIST_OF_SANITIZERS)
            if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
                target_compile_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
                target_link_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
                message(STATUS "Sanitizers habilitados: ${LIST_OF_SANITIZERS}")
            endif()
        endif()
        
    endif()

endfunction()
