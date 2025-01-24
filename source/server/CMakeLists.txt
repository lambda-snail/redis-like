#find_package(Boost 1.83 REQUIRED)

add_library(server)
target_sources(server
        PUBLIC
        FILE_SET CXX_MODULES FILES
        server.cppm
        database.cpp
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wformat=2 -Wconversion -Wimplicit-fallthrough")
    target_compile_options(server PRIVATE -Wall -Wformat=2 -Wconversion -Wimplicit-fallthrough)
endif()

add_library(LambdaSnail::server ALIAS server)

target_link_libraries(server PRIVATE LambdaSnail::memory LambdaSnail::resp spdlog TBB::tbb)

target_link_libraries(server PUBLIC TracyClient)
target_include_directories(server PUBLIC ${Tracy_SOURCE_DIR}/public)