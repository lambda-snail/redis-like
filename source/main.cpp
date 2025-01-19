module;

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

inline std::shared_ptr<LambdaSnail::logging::logger> logger;
inline runner runner;

void signalHandler( int signum ) {
    logger->get_system_logger()->critical("Termination signal received");
    runner.shutdown();
    logger.reset(); // Force the logger to clean up

    exit(signum);
}

int main(int argc, char const** argv)
{
    ZoneScoped;

    logger = std::make_shared<LambdaSnail::logging::logger>();
    logger->init_logger(argc, argv);

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    LambdaSnail::server::database dispatch{allocator};

    signal(SIGINT, signalHandler);

    runner.run(6379, dispatch, buffer_pool);

    return 0;
}
