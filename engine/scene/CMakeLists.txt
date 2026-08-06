add_library(ri_scene STATIC scene.c)
add_library(Riemenn::Scene ALIAS ri_scene)

target_include_directories(ri_scene PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_scene PUBLIC ri_ecs ri_math)
