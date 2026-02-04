add_library(bhs_math STATIC)

# Coletar fontes recursivamente
file(GLOB_RECURSE MATH_SOURCES "*.c")
list(FILTER MATH_SOURCES EXCLUDE REGEX "tests/.*")

target_sources(bhs_math PRIVATE ${MATH_SOURCES})

# Includes
target_include_directories(bhs_math PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_SOURCE_DIR}
)

# Dependências (Math Lib)
target_link_libraries(bhs_math PUBLIC m)

set_project_warnings(bhs_math)

add_library(BHS::Math ALIAS bhs_math)
