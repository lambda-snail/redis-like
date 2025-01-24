#find_package(Boost 1.83 REQUIRED)

add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
        database.cpp
)

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::memory LambdaSnail::resp spdlog TBB::tbb)

target_link_libraries(server PUBLIC TracyClient)
target_include_directories(server PUBLIC ${Tracy_SOURCE_DIR}/public)