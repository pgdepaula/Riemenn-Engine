# tests/CMakeLists.txt

# Math Unit Tests
if(EXISTS "${CMAKE_SOURCE_DIR}/math/tests/test_core.c")
    add_executable(unit_tests_math "${CMAKE_SOURCE_DIR}/math/tests/test_core.c")
    target_link_libraries(unit_tests_math PRIVATE bhs_math) 
    target_include_directories(unit_tests_math PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME MathCoreTest COMMAND unit_tests_math)
endif()

# Gravity Logic Logic Test (New)
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/test_gravity_logic.c")
    add_executable(test_gravity_logic "${CMAKE_SOURCE_DIR}/tests/unit/test_gravity_logic.c")
    # No linking needed as it is a standalone logic test with mocks
    target_include_directories(test_gravity_logic PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME GravityLogicTest COMMAND test_gravity_logic)
endif()

# Global Integration Tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_scene_lifecycle.c")
    add_executable(integration_tests "${CMAKE_CURRENT_SOURCE_DIR}/test_scene_lifecycle.c")
    target_link_libraries(integration_tests PRIVATE bhs_engine bhs_gui bhs_math)
    target_include_directories(integration_tests PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME SceneLifecycleTest COMMAND integration_tests)
endif()

# SVG Reproduction Test
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/repro_issue.c")
    add_executable(repro_svg_issue "${CMAKE_CURRENT_SOURCE_DIR}/repro_issue.c")
    target_link_libraries(repro_svg_issue PRIVATE bhs_engine bhs_math)
    target_include_directories(repro_svg_issue PRIVATE ${CMAKE_SOURCE_DIR})
    add_test(NAME SVGReproTest COMMAND repro_svg_issue)
endif()

# Legacy Manual Test
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_svg_manual.c")
    add_executable(test_svg_manual "${CMAKE_CURRENT_SOURCE_DIR}/test_svg_manual.c")
    target_link_libraries(test_svg_manual PRIVATE bhs_engine bhs_math)
    target_include_directories(test_svg_manual PRIVATE ${CMAKE_SOURCE_DIR})
endif()

message(STATUS "Testes configurados.")
