# Lua PUC Bindings Module
cmake_minimum_required(VERSION 3.16)

# Try to find Lua (5.1+ usually)
# Prefer 5.3 or 5.4, but accept 5.1/LuaJIT as fallback for headers
find_package(Lua)

if(NOT LUA_FOUND)
    message(WARNING "Lua not found. Lua PUC bindings (riengine_lua) will NOT be built.")
    return()
endif()

message(STATUS "Lua found: ${LUA_VERSION_STRING} at ${LUA_INCLUDE_DIR}")

# Create the module
# MODULE libraries are plugins that are not linked by other targets but loaded dynamically
add_library(riengine_lua MODULE ../../include/lua/puc/riengine_lua.c)

# Link against the main engine library
target_link_libraries(riengine_lua PRIVATE riengine ${LUA_LIBRARIES})

# Include directories
target_include_directories(riengine_lua PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include 
    ${LUA_INCLUDE_DIR}
)

# Set prefix to empty so we get 'riengine.so' (or .dll) instead of 'libriengine.so' if desired for require 'riengine'
# But usually C modules are named 'name.so'. require "riengine" looks for riengine.so.
set_target_properties(riengine_lua PROPERTIES PREFIX "")
set_target_properties(riengine_lua PROPERTIES OUTPUT_NAME "riengine")

# Add to main build if desired, or just sit there.
# The user's prompt implies we should integrate it. 
# We'll need to add_subdirectory(lua/puc) in the main CMakeLists.txt if we want it built by default.
