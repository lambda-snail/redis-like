cmake_minimum_required(VERSION 3.29)

project(
    redis_like
    VERSION "0.4.0"
    LANGUAGES CXX
)

add_compile_definitions(LAMBDA_SNAIL_VERSION="${CMAKE_PROJECT_VERSION}")

set(CMAKE_CXX_SCAN_FOR_MODULES 1)
set(CMAKE_CXX_STANDARD 23)

option(BUILD_TESTS "Enable testing" OFF)
option(SANITIZE_ADDRESS "Use address sanitizer" OFF)
option(SANITIZE_MEMORY "Use memory sanitizer" OFF)
option(SANITIZE_THREAD "Use thread sanitizer" OFF)
option(SANITIZE_UB "Use ub sanitizer" OFF)

message("Tracy is: ${TRACY_ENABLE}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

add_subdirectory(source)
add_subdirectory(external)

#include(cmake/IncludeMoodycamel.cmake)
#include(cmake/IncludeTBB.cmake)
include(cmake/IncludeCli11.cmake)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()