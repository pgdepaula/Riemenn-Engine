file(GLOB_RECURSE MATH_SOURCES "*.c")
# Exclude tests if present
list(FILTER MATH_SOURCES EXCLUDE REGEX "tests/.*")

add_library(ri_math STATIC ${MATH_SOURCES})
add_library(Riemenn::Math ALIAS ri_math)

target_include_directories(ri_math PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ri_math PUBLIC ri_foundation m)
