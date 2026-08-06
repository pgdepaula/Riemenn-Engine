add_library(ri_foundation STATIC log.c)
add_library(Riemenn::Foundation ALIAS ri_foundation)

target_include_directories(ri_foundation PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_foundation PRIVATE pthread)
