add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
)

target_sources(server
        PUBLIC
        database.cpp
        timeout_worker.cpp
)

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::logging LambdaSnail::memory LambdaSnail::resp TBB::tbb)

target_link_libraries(server PUBLIC TracyClient)
target_include_directories(server PUBLIC ${Tracy_SOURCE_DIR}/public)