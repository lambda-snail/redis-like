add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
)

target_sources(server
        PUBLIC
        command_dispatch.cpp
        database.cpp
        server.cpp
        timeout_worker.cpp
)

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::logging LambdaSnail::memory LambdaSnail::resp)

target_link_libraries(server PUBLIC TracyClient)
target_include_directories(server PUBLIC ${Tracy_SOURCE_DIR}/public)