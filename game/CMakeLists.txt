file(GLOB_RECURSE GAME_SOURCES "*.c")
# Exclude assets directory if it contains any .c files
list(FILTER GAME_SOURCES EXCLUDE REGEX "assets/.*")

add_executable(blackhole_sim ${GAME_SOURCES})

target_include_directories(blackhole_sim PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(blackhole_sim PRIVATE ri_engine pthread m dl)

# Copy assets to output directory
file(COPY assets/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)
