add_library(ri_assets STATIC image_loader.c svg_loader.c image_gen.c)
add_library(Riemenn::Assets ALIAS ri_assets)

target_include_directories(ri_assets PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_assets PUBLIC ri_foundation)
