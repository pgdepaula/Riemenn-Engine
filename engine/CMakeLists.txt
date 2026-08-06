add_subdirectory(foundation)
add_subdirectory(math)
add_subdirectory(platform)
add_subdirectory(rhi)
add_subdirectory(ecs)
add_subdirectory(physics)
add_subdirectory(assets)
add_subdirectory(geometry)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(ui)

add_library(ri_engine INTERFACE)
add_library(Riemenn::Engine ALIAS ri_engine)
target_link_libraries(ri_engine INTERFACE
    ri_foundation ri_math ri_platform ri_rhi ri_ecs ri_physics
    ri_assets ri_geometry ri_render ri_scene ri_ui
)
target_include_directories(ri_engine INTERFACE ${CMAKE_SOURCE_DIR})
