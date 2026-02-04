# engine/CMakeLists.txt

add_library(bhs_engine STATIC)

# Fontes recursivos
file(GLOB_RECURSE ENGINE_SOURCES "*.c")
# Remover arquivos que podem ser testes ou ferramentas se houver
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "tests/.*")

target_sources(bhs_engine PRIVATE ${ENGINE_SOURCES})

# Includes Públicos
target_include_directories(bhs_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Dependências (Engine usa gui e Math)
target_link_libraries(bhs_engine PUBLIC 
    bhs_gui
    bhs_math
)

set_project_warnings(bhs_engine)

add_library(BHS::Engine ALIAS bhs_engine)
