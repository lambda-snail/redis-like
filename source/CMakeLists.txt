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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(redis-like PRIVATE -Wall -Wformat=2 -Wconversion -Wimplicit-fallthrough)

    if(SANITIZE_ADDRESS)
        target_compile_options(redis-like PRIVATE -fsanitize=address)
    endif()
    if(SANITIZE_THREAD)
        target_compile_options(redis-like PRIVATE -fsanitize=thread)
    endif()
    if(SANITIZE_MEMORY)
        target_compile_options(redis-like PRIVATE -fsanitize=memory)
    endif()
    if(SANITIZE_UB)
        target_compile_options(redis-like PRIVATE -fsanitize=undefined)
    endif()
endif()

target_link_libraries(redis-like
        PRIVATE
#        asio

        LambdaSnail::resp
        LambdaSnail::networking
        LambdaSnail::server
        LambdaSnail::logging
)
