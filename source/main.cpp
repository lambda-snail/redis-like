module;

#include <csignal>

#include <tracy/Tracy.hpp>

#include <CLI/CLI.hpp>

export module main;

import logging;
import memory;
import networking;
import resp;
import server;

struct cmd_arguments
{
    uint16_t port{ 6379 };
};

std::unique_ptr<cmd_arguments> add_arguments(CLI::App& app, int argc, char const** argv)
{
    auto options = std::make_unique<cmd_arguments>();

    app.add_option<uint16_t>("-p,--port", options->port, "The port to listen at");

    return options;
}


int main(int argc, char const** argv)
{
    ZoneScoped;

    CLI::App app{"A key-value store with a partial implementation of the RESP protocol."};
    //auto utf8_args = app.ensure_utf8(argv); // Only on Windows
    auto options = add_arguments(app, argc, argv);
    CLI11_PARSE(app, argc, argv);

    auto logger = std::make_shared<LambdaSnail::logging::logger>();
    logger->init_logger(argc, argv);

    logger->get_system_logger()->info("The server is starting, the version is {}", LAMBDA_SNAIL_VERSION);

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    LambdaSnail::server::database dispatch{allocator};

    runner runner(logger);
    //runner.run(6379, dispatch, buffer_pool);
    runner.run(options->port, dispatch, buffer_pool);

    return 0;
}
