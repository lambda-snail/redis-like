module;

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ts/internet.hpp>
#include <asio/co_spawn.hpp>
#include <asio/deferred.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/placeholders.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <cstdio>

#include <exception>

#include <csignal>

#include <tracy/Tracy.hpp>

export module networking :resp.tcp_server;

import logging;
import memory;
import server;
import resp;

using default_token_t = asio::deferred_t;
using tcp_acceptor_t = default_token_t::as_default_on_t<asio::ip::tcp::acceptor>;
using tcp_socket_t = default_token_t::as_default_on_t<asio::ip::tcp::socket>;

asio::awaitable<void> connection(
    tcp_socket_t socket,
    std::shared_ptr<LambdaSnail::server::database> database,
    LambdaSnail::memory::buffer_pool& buffer_pool,
    std::shared_ptr<LambdaSnail::logging::logger> logger)
{
    logger->get_network_logger()->trace("Connection received on port: {}", socket.remote_endpoint().port());

    auto buffer_info = buffer_pool.request_buffer(2048);
    if (buffer_info.size == 0)
    {
        // TODO: Add own concrete exception here
        logger->get_network_logger()->error("Failed to acquire memory from buffer pool");
        throw std::bad_alloc();
    }

    try
    {
        while (true)
        {
            auto [ec, n] = co_await socket.async_read_some(
                asio::buffer(buffer_info.buffer, buffer_info.size),
                asio::as_tuple(asio::use_awaitable));
            if (ec == asio::error::eof) [[unlikely]]
            {
                // We separate the handling of eof since it's not an error per se
                break;
            }

            if (ec) [[unlikely]]
            {
                // These are the errors we should log
                logger->get_network_logger()->error("Error while reading from socket: {}", ec.message());
                break;
            }

            LambdaSnail::resp::data_view resp_data(std::string_view(buffer_info.buffer, n));
            std::string response = database->process_command(resp_data);

            auto [ec_w, n_written] = co_await async_write(socket, asio::buffer(response, response.size()), asio::as_tuple(asio::use_awaitable));
            if (ec) [[unlikely]]
            {
                logger->get_network_logger()->error("Error while writing to socket: {}", ec_w.message());
            }
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }

    buffer_pool.release_buffer(buffer_info.buffer);
}

asio::awaitable<void> listener(
    uint16_t port,
    std::shared_ptr<LambdaSnail::server::database> database,
    LambdaSnail::memory::buffer_pool& buffer_pool,
    std::shared_ptr<LambdaSnail::logging::logger> logger)
{
    auto executor = co_await asio::this_coro::executor;
    tcp_acceptor_t acceptor(executor, {asio::ip::tcp::v4(), port});

#ifndef _WIN32
    // acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    // typedef asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;
    // acceptor.set_option(reuse_port(true));
#endif

    while (true)
    {
        auto [ec, socket] = co_await acceptor.async_accept(asio::as_tuple(asio::use_awaitable));
        if (ec)
        {
            logger->get_network_logger()->error("Error listening for connections: {}", ec.message());
            break;
        }

        co_spawn(executor, connection(std::move(socket), database, buffer_pool, logger), asio::detached);
    }
}

export class tcp_server
{
public:
    tcp_server(
        LambdaSnail::server::timeout_worker& maintenance_thread,
        std::shared_ptr<LambdaSnail::logging::logger> logger)
    :
        m_logger(logger),
        m_maintenance_timer(m_context),
        m_maintenance_thread(maintenance_thread)
    {
    }

    void run(
        uint16_t port,
        std::shared_ptr<LambdaSnail::server::database> database,
        LambdaSnail::memory::buffer_pool &buffer_pool)
    {
        ZoneScoped;

        try
        {
            asio::signal_set signal_set{m_context};
            signal_set.add(SIGINT);
            signal_set.add(SIGTERM);
#if defined(SIGHUP)
            signal_set.add(SIGHUP);
#endif
#if defined(SIGQUIT)
            signal_set.add(SIGQUIT);
#endif
            signal_set.async_wait(
                [&](std::error_code ec, int signal)
                {
#ifndef _WIN32
                    m_logger->get_system_logger()->info("The system received signal {} - {}", signal, strsignal(signal));
#else
                    m_logger->get_system_logger()->info("The system received signal {}", signal);
#endif
                    m_maintenance_timer.cancel();
                    m_context.stop();
                });

            asio::co_spawn(m_context, listener(port, database, buffer_pool, m_logger), asio::detached);

            m_maintenance_timer.expires_after(asio::chrono::seconds(10));
            m_maintenance_timer.async_wait(std::bind(&tcp_server::maintenance_timer_handler, this, std::placeholders::_1));

            m_context.run();

            // If we are using an asio::thread_pool, we should join here to wait for
            // asio to be done (i.e., server shutdown)
            // m_context.join();
        } catch (std::exception &e)
        {
            m_logger->get_system_logger()->error("Exception in server runner: {}", e.what());
        }
    }

    ~tcp_server()
    {
        m_logger->get_system_logger()->info("The server is shutting down");
    }

private:
    asio::io_context m_context { 1 };
    //asio::thread_pool m_context { m_thread_pool_size };

    std::shared_ptr<LambdaSnail::logging::logger> m_logger;

    asio::steady_timer m_maintenance_timer;
    LambdaSnail::server::timeout_worker& m_maintenance_thread;
    void maintenance_timer_handler(std::error_code const ec)
    {
        if (not ec)
        {
            m_maintenance_thread.do_work();
            m_maintenance_timer.expires_after(asio::chrono::seconds(10));
            m_maintenance_timer.async_wait(std::bind(&tcp_server::maintenance_timer_handler, this, std::placeholders::_1));
        }
        else
        {
            this->m_logger->get_network_logger()->error("Error encountered by maintenance timer: {}", ec.message());
        }
    }
};
