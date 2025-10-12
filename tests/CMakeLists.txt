set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Build with tests enabled")

include(FetchContent)
#FetchContent_Declare(
#        googletest
#        GIT_REPOSITORY https://github.com/google/googletest.git
#        GIT_TAG        v1.15.2
#)
## For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        fuzztest
        GIT_REPOSITORY https://github.com/google/fuzztest.git
        GIT_TAG        2025-07-28
)
FetchContent_MakeAvailable(fuzztest)

enable_testing()
include(GoogleTest)

add_executable(
        redis-like-tests
        parser_tests.cpp
        server_tests.cpp
)

target_include_directories(redis-like-tests PUBLIC ${fuzztest_SOURCE_DIR})

#target_compile_options(LambdaSnail::resp PRIVATE -fsanitize=address -g -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -UNDEBUG -fsanitize-coverage=inline-8bit-counters -fsanitize-coverage=trace-cmp -DADDRESS_SANITIZER)
#target_link_libraries(LambdaSnail::resp PRIVATE -fsanitize=address)
fuzztest_setup_fuzzing_flags()

target_link_libraries(
        redis-like-tests
        PRIVATE
        LambdaSnail::resp
        LambdaSnail::server
)

# Includes GTest::gtest_main
link_fuzztest(redis-like-tests)
gtest_discover_tests(redis-like-tests)