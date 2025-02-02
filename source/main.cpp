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

std::unique_ptr<LambdaSnail::networking::server_options> add_arguments(CLI::App& app, int argc, char const** argv)
{
    auto options = std::make_unique<LambdaSnail::networking::server_options>();

    app.add_option<uint16_t>("-p,--port", options->port, "The port to listen at");
    app.add_option<uint32_t>("--ci,--cleanup-interval", options->cleanup_interval_seconds, "The number of seconds between each check for deleted entries");

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

    auto database = std::make_shared<LambdaSnail::server::database>();

    LambdaSnail::server::timeout_worker maintenance_thread(logger);
    maintenance_thread.add_database(database);

    tcp_server runner(maintenance_thread, logger, std::move(options));
    //runner.run(6379, dispatch, buffer_pool);
    runner.run(database, buffer_pool);

    return 0;
}
