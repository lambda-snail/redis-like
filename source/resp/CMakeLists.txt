add_library(resp)
target_sources(resp
        PUBLIC
        FILE_SET CXX_MODULES FILES
        parser.cpp
        resp.cppm
)

add_library(LambdaSnail::resp ALIAS resp)

target_link_libraries(resp PUBLIC TracyClient)
target_include_directories(resp PUBLIC ${Tracy_SOURCE_DIR}/public)