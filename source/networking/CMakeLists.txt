include(FetchContent)

# Setup Asio
# Based on cmake file provided by havogt:
# https://stackoverflow.com/questions/65586352/is-it-possible-to-use-fetchcontent-or-an-equivalent-to-add-a-library-that-has-no

FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
)

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_MakeAvailable(asio)
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

find_package(Threads)
target_link_libraries(asio INTERFACE Threads::Threads)

add_compile_definitions(ASIO_NO_DEPRECATED)
add_compile_definitions(ASIO_STANDALONE)

add_compile_definitions(ASIO_HAS_STD_ADDRESSOF)
add_compile_definitions(ASIO_HAS_STD_ARRAY)
add_compile_definitions(ASIO_HAS_CSTDINT)
add_compile_definitions(ASIO_HAS_STD_SHARED_PTR)
add_compile_definitions(ASIO_HAS_STD_TYPE_TRAITS)

add_library(networking)
target_sources(networking
        PUBLIC
        FILE_SET CXX_MODULES FILES
        tcp_server.cpp
        networking.cppm
)

add_library(LambdaSnail::networking ALIAS networking)

target_link_libraries(networking PRIVATE LambdaSnail::logging LambdaSnail::memory LambdaSnail::resp LambdaSnail::server spdlog)

target_link_libraries(networking PUBLIC TracyClient)
target_include_directories(networking PUBLIC ${Tracy_SOURCE_DIR}/public)