add_library(ri_render STATIC camera.c camera_controller.c render_system.c)
add_library(Riemenn::Render ALIAS ri_render)

target_include_directories(ri_render PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_render PUBLIC ri_rhi ri_math)
