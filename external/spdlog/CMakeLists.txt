include(FetchContent)

FetchContent_Declare (
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(redis-like PRIVATE spdlog::spdlog)