add_library(logging)
target_sources(logging
        PUBLIC
        FILE_SET CXX_MODULES FILES
        logger.cpp
        logging.cppm
)

add_library(LambdaSnail::logging ALIAS logging)
target_link_libraries(logging PUBLIC spdlog::spdlog)