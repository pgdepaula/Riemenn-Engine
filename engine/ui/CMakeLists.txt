file(GLOB_RECURSE UI_SOURCES "*.c")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
pkg_check_modules(FREETYPE REQUIRED freetype2)

add_library(ri_ui STATIC ${UI_SOURCES})
add_library(Riemenn::UI ALIAS ri_ui)

target_include_directories(ri_ui PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(ri_ui PRIVATE ${FONTCONFIG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(ri_ui PUBLIC ri_foundation ri_rhi ri_platform)
target_link_libraries(ri_ui PRIVATE ${FONTCONFIG_LIBRARIES} ${FREETYPE_LIBRARIES})
