add_library(memory)
target_sources(memory
        PUBLIC
        FILE_SET CXX_MODULES FILES
        memory.cppm
        buffer_allocator.cpp
)

add_library(LambdaSnail::memory ALIAS memory)

#target_link_libraries(memory PRIVATE concurrentqueue)