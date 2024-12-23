
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wformat=2 -Wconversion -Wimplicit-fallthrough")

    # SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,memory")

    #elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

# Own application and modules

add_subdirectory(resp)
add_subdirectory(networking)
add_subdirectory(server)

add_executable(redis-like main.cpp)
target_sources(redis-like
        PUBLIC
        FILE_SET CXX_MODULES FILES
        main.cpp
)

#redis_like_target_properties(resp)

target_include_directories(redis-like PUBLIC ${readerwriterqueue_SOURCE_DIR})
target_include_directories(redis-like PUBLIC ${concurrentqueue_SOURCE_DIR})

target_link_libraries(redis-like
        PRIVATE
        asio
        readerwriterqueue
        concurrentqueue

        LambdaSnail::resp
        LambdaSnail::networking
        LambdaSnail::server
)
