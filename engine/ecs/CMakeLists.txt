add_library(ri_ecs STATIC ecs.c events.c)
add_library(Riemenn::ECS ALIAS ri_ecs)

target_include_directories(ri_ecs PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_ecs PUBLIC ri_foundation ri_math)
