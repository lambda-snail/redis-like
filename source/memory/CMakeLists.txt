add_library(memory_module)
target_sources(memory_module
        PUBLIC
        FILE_SET CXX_MODULES FILES
        memory.cppm
)

target_sources(memory_module
        PUBLIC
        buffer_pool.cpp
)

add_library(LambdaSnail::memory ALIAS memory_module)