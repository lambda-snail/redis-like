set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Build with tests enabled")

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        redis-like-tests
        parser_tests.cpp
)
target_link_libraries(
        redis-like-tests
        LambdaSnail::resp
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(redis-like-tests)
