cmake_minimum_required(VERSION 3.29)

project(
    redis_like
    VERSION "0.2.0"
    LANGUAGES CXX
)

set(CMAKE_CXX_SCAN_FOR_MODULES 1)
set(CMAKE_CXX_STANDARD 23)

option(USE_TRACY "Enable tracing" OFF)
option(BUILD_TESTS "Enable testing" OFF)
option(SANITIZE_ADDRESS "Use address sanitizer" OFF)
option(SANITIZE_MEMORY "Use memory sanitizer" OFF)
option(SANITIZE_THREAD "Use thread sanitizer" OFF)
option(SANITIZE_UB "Use ub sanitizer" OFF)

if(NOT USE_TRACY)
    set(TRACY_ENABLE OFF)
endif()

message("Tracy is: ${TRACY_ENABLE}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wformat=2 -Wconversion -Wimplicit-fallthrough")

    if(SANITIZE_ADDRESS)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    endif()
    if(SANITIZE_THREAD)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    endif()
    if(SANITIZE_MEMORY)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
    endif()
    if(SANITIZE_UB)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=libc++")
endif()

add_subdirectory(source)
add_subdirectory(external)

include(cmake/IncludeMoodycamel.cmake)
include(cmake/IncludeLibcuckoo.cmake)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
