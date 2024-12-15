
add_library(resp)
target_sources(resp
        PUBLIC
        FILE_SET CXX_MODULES FILES
        parser.cpp
        resp.cpp
)

#target_link_libraries(resp PRIVATE fmt::fmt spdlog::spdlog)
add_library(LambdaSnail::resp ALIAS resp)

add_executable(redis-like main.cpp)

#redis_like_target_properties(resp)

target_link_libraries(redis-like PRIVATE LambdaSnail::resp)