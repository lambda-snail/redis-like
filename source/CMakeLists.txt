# Own application and modules

add_subdirectory(resp)
add_subdirectory(memory)
add_subdirectory(networking)
add_subdirectory(server)
add_subdirectory(logging)

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
        #spdlog

        LambdaSnail::resp
        LambdaSnail::networking
        LambdaSnail::server
        LambdaSnail::logging
)
