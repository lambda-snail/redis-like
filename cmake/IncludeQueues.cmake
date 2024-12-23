include(FetchContent)

# readerwriterqueue

FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
    GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

# concurrentqueue

FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY    https://github.com/cameron314/concurrentqueue
    GIT_TAG           master
)

FetchContent_MakeAvailable(concurrentqueue)
