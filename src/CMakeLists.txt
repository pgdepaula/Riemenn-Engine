# src/CMakeLists.txt

# Executável Final
add_executable(blackhole_sim)

# Fontes
file(GLOB_RECURSE APP_SOURCES "*.c")

# Remover assets do glob source se pegou algo errado (embora assets so tenha .c se for shader)
# Vamos separar explicitamente se precisarmos, mas por enquanto glob .c é apenas codigo C host.
# Se os shaders estao em src/assets/shaders/*.c, eles serao pegos aqui como fonte C normal?
# O compilador C vai tentar compilar kernels OpenCL como C e falhar se tiver syntax especifica.
# O makefile original filtrava SHADER_SRC_DIR := src/assets/shaders.
# Devemos excluir shaders da compilação do HOST.

# Filtrar shaders
list(FILTER APP_SOURCES EXCLUDE REGEX "assets/shaders/.*")

target_sources(blackhole_sim PRIVATE ${APP_SOURCES})

# Includes
target_include_directories(blackhole_sim PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Linkagem
target_link_libraries(blackhole_sim PRIVATE 
    bhs_engine
    bhs_gui
    bhs_math
)

# Threads (comum em linux)
target_link_libraries(blackhole_sim PRIVATE pthread m dl)

set_project_warnings(blackhole_sim)

# Copiar Assets (se houver imagens, conf etc)
# assets/ está na raiz do repositorio, nao em src/assets (exceto shaders)
# O makefile tinha SHADER_OUT_DIR := assets/shaders na raiz.
# Vamos copiar a pasta assets da raiz para o build dir para rodar facil.
file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Compilação de Shaders (Chamada ao módulo)
target_compile_shaders(blackhole_sim)

