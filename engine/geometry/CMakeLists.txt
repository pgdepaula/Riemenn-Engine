add_library(ri_geometry STATIC mesh_gen.c)
add_library(Riemenn::Geometry ALIAS ri_geometry)

target_include_directories(ri_geometry PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_geometry PUBLIC ri_math)
