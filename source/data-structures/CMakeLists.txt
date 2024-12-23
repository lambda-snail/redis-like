include(FetchContent)

# readerwriterqueue

FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
    GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_GetProperties(readerwriterqueue)
if(NOT readerwriterqueue_POPULATED)
    FetchContent_Populate(readerwriterqueue)
endif()

# concurrentqueue

FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY    https://github.com/cameron314/concurrentqueue
    GIT_TAG           master
)

FetchContent_MakeAvailable(concurrentqueue)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

add_library(data-structures)
target_sources(data-structures
        PUBLIC
        FILE_SET CXX_MODULES FILES
        data_structures.cppm
        spsc_queue.cpp
        mpmc_queue.cpp
)

target_include_directories(data-structures PRIVATE ${readerwriterqueue_SOURCE_DIR})
target_include_directories(data-structures PRIVATE ${concurrentqueue_SOURCE_DIR})

target_link_libraries(resp PRIVATE readerwriterqueue concurrentqueue)
add_library(LambdaSnail::data-structures ALIAS data-structures)