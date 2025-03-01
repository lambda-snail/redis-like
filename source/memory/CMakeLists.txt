add_library(memory)
target_sources(memory
        PUBLIC
        FILE_SET CXX_MODULES FILES
        memory.cppm
)

target_sources(memory
        PUBLIC
        buffer_allocator.cpp
        buffer_pool.cpp
)

add_library(LambdaSnail::memory ALIAS memory)