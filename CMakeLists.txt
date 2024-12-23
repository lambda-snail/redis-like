cmake_minimum_required(VERSION 3.29)

project(
    redis_like
    VERSION "0.0.0"
    LANGUAGES CXX
)

set(CMAKE_CXX_SCAN_FOR_MODULES 1)
set(CMAKE_CXX_STANDARD 23)

option(BUILD_TESTS "Enable testing" OFF)

add_subdirectory(source)

include(cmake/IncludeMoodycamel.cmake)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()