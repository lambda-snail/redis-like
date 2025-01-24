module;

#include <csignal>
#include <iostream>

#include <spdlog/sinks/daily_file_sink.h>

#include <tracy/Tracy.hpp>

export module main;

import logging;
import memory;
import networking;
import resp;
import server;

int main(int argc, char const** argv)
{
    ZoneScoped;

    auto logger = std::make_shared<LambdaSnail::logging::logger>();
    logger->init_logger(argc, argv);

    logger->get_system_logger()->info("The server is starting, the version is {}", LAMBDA_SNAIL_VERSION);

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    LambdaSnail::server::database dispatch{allocator};

    runner runner(logger);
    runner.run(6379, dispatch, buffer_pool);

    return 0;
}
