file(GLOB_RECURSE PHYSICS_SOURCES "*.c")

add_library(ri_physics STATIC ${PHYSICS_SOURCES})
add_library(Riemenn::Physics ALIAS ri_physics)

target_include_directories(ri_physics PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_physics PUBLIC ri_foundation ri_math)
