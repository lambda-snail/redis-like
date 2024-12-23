add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
        command_dispatch.cpp
)

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::resp)