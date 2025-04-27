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

    // To allow arguments to be passed to spdlog we need to ignore extra commands
    // It may not be the best thing to expose spdlog arguments directly, but for
    // now it works well
    app.allow_extras();

    app.add_option<uint16_t>("-p,--port", options->port, "The port to listen at")->capture_default_str();
    app.add_option<uint32_t>("--ci,--cleanup-interval", options->cleanup_interval_seconds, "The number of seconds between each check for deleted entries")->capture_default_str();
    app.add_option<uint8_t>("-n,--num-databases", options->num_databases, "The number of databases (namespaces) to create in the server")->capture_default_str();

    return options;
}


int main(int argc, char const** argv)
{
    ZoneScoped;

    CLI::App app{"A key-value store with a partial implementation of the RESP protocol."};
    //auto utf8_args = app.ensure_utf8(argv); // Needed on Windows
    auto options = add_arguments(app, argc, argv);
    CLI11_PARSE(app, argc, argv);

    auto logger = std::make_shared<LambdaSnail::logging::logger>();
    logger->init_logger(argc, argv);

    logger->get_system_logger()->info("The server is starting, the version is {}", LAMBDA_SNAIL_VERSION);

    LambdaSnail::memory::buffer_pool buffer_pool{};

    LambdaSnail::server::server server(options->num_databases);

    LambdaSnail::server::timeout_worker maintenance_thread(server, logger);

    tcp_server runner(server, maintenance_thread, logger, std::move(options));
    runner.run(buffer_pool); // TODO: Move parameter to ctor

    return 0;
}