# gui/tests/CMakeLists.txt

# Função auxiliar para criar executáveis de teste
function(bhs_add_gui_framework_test name source)
    add_executable(${name} ${source})
    
    # Linkar com gui e Math
    target_link_libraries(${name} PRIVATE bhs_gui bhs_math)
    
    # Includes: Raiz do projeto (para incluir "gui/...") e diretório atual (test_runner.h)
    target_include_directories(${name} PRIVATE 
        ${CMAKE_SOURCE_DIR} 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Adicionar ao CTest
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# --- Testes ---

# 1. Ciclo de Vida (Init/Shutdown, Janelas, Leaks)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lifecycle.c")
    bhs_add_gui_framework_test(test_lifecycle test_lifecycle.c)
endif()

# 2. Input System (Polling, Mouse, Keyboard)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_input.c")
    bhs_add_gui_framework_test(test_input test_input.c)
endif()

# 3. RHI Boot (Vulkan Device Creation)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_rhi_boot.c")
    bhs_add_gui_framework_test(test_rhi_boot test_rhi_boot.c)
endif()

message(STATUS "Testes do gui configurados.")
