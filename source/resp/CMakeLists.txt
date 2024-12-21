add_library(resp)
target_sources(resp
        PUBLIC
        FILE_SET CXX_MODULES FILES
        parser.cpp
        resp.cppm
)

#target_link_libraries(resp PRIVATE fmt::fmt spdlog::spdlog)
add_library(LambdaSnail::resp ALIAS resp)