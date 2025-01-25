# CMake script adapted from github issue by dbs4261:
# https://github.com/uxlfoundation/oneTBB/issues/1323

include(FetchContent)
FetchContent_Declare(TBB
        GIT_REPOSITORY "https://github.com/oneapi-src/oneTBB.git"
        GIT_TAG "master"
        GIT_SHALLOW ON
        CMAKE_ARGS "-DTBB_TEST:BOOL=OFF -DTBB_EXAMPLES:BOOL=OFF -DTBB_BENCH:BOOL=OFF -DTBB_BUILD:BOOL=ON -DTBB_FIND_PACKAGE:BOOL=OFF -DTBB_FUZZ_TESTING:BOOL=OFF -DTBB_INSTALL:BOOL=ON"
        OVERRIDE_FIND_PACKAGE
)

set(TBB_TEST OFF CACHE INTERNAL "" FORCE)
set(TBB_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(TBB_BENCH OFF CACHE INTERNAL "" FORCE)
set(TBB_BUILD ON CACHE INTERNAL "" FORCE)
set(TBB_FIND_PACKAGE OFF CACHE INTERNAL "" FORCE)
set(TBB_FUZZ_TESTING OFF CACHE INTERNAL "" FORCE)
set(TBB_INSTALL ON CACHE INTERNAL "" FORCE)

FetchContent_MakeAvailable(TBB)

target_link_directories(redis-like PRIVATE TBB::tbb)
