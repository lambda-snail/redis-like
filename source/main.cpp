module;

#include <csignal>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/cfg/argv.h>
#include <spdlog/cfg/env.h>

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

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    LambdaSnail::server::database dispatch{allocator};

    runner runner(logger);
    runner.run(6379, dispatch, buffer_pool);

    return 0;
}
