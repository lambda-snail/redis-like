module;

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ts/internet.hpp>
#include <asio/co_spawn.hpp>
#include <asio/deferred.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <cstdio>

#include <exception>
#include <iostream>
#include <thread>

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

asio::awaitable<void> echo(
    tcp_socket_t socket,
    LambdaSnail::server::database& dispatch,
    LambdaSnail::memory::buffer_pool& buffer_pool)
{
    std::cout << "Connection received on port " << socket.remote_endpoint().port() << std::endl;

    auto buffer_info = buffer_pool.request_buffer(2048);
    if (buffer_info.size == 0)
    {
        std::cout << "No buffers available" << std::endl;
    }

    try
    {
        //char data[2048];
        while (true) // TODO: This is for streaming later
        {
            auto [ec, n] = co_await socket.async_read_some(
                asio::buffer(buffer_info.buffer, buffer_info.size),
                asio::as_tuple(asio::use_awaitable));
            if (ec)
            {
                std::cout << ec.message() << std::endl;
                break;
                //buffer_pool.release_buffer(buffer_info.buffer);
                //co_return;
            }

            LambdaSnail::resp::data_view resp_data(std::string_view(buffer_info.buffer, n));
            std::string response = dispatch.process_command(resp_data);

            //std::string response = "+OK\r\n";

            auto [ec_w, n_written] = co_await async_write(socket, asio::buffer(response, response.size()), asio::as_tuple(asio::use_awaitable));
            if (ec)
            {
                std::cout << ec_w.message() << std::endl;
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
    LambdaSnail::server::database& dispatch,
    LambdaSnail::memory::buffer_pool& buffer_pool)
{
    auto executor = co_await asio::this_coro::executor;
    tcp_acceptor_t acceptor(executor, {asio::ip::tcp::v4(), port});

    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
     typedef asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;
     acceptor.set_option(reuse_port(true));

    // int one = 1;
    // auto result = setsockopt(acceptor.native_handle(), SOL_SOCKET /*SOL_SOCKET*/, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
    //auto error = strerror(errno); //errno;

    while (true)
    {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept();
        co_spawn(executor, echo(std::move(socket), dispatch, buffer_pool), asio::detached);
    }
}

// TODO: Move this out from here
export class runner
{
public:
    runner(std::shared_ptr<LambdaSnail::logging::logger> logger) : m_logger(logger)
    {
    }

    void run(uint16_t port, LambdaSnail::server::database &dispatch, LambdaSnail::memory::buffer_pool &buffer_pool)
    {
        ZoneScoped;

        try
        {
            asio::signal_set signal_set{m_context};
            signal_set.add(SIGINT);
            signal_set.add(SIGTERM);
            signal_set.add(SIGHUP);
#if defined(SIGQUIT)
            signal_set.add(SIGQUIT);
#endif
            signal_set.async_wait(
                [this](std::error_code ec, int signal)
                {
                    m_logger->get_system_logger()->
                            info("The system received signal {} - {}", signal, strsignal(signal));
                    m_context.stop();
                });

            asio::co_spawn(m_context, listener(port, dispatch, buffer_pool), asio::detached);

            m_context.run();

            // tcp_server server(m_context, port, dispatch, buffer_pool);
            //
            // // See https://think-async.com/Asio/asio-1.30.2/src/examples/cpp11/http/server3/server.cpp
            // for (std::size_t i = 0; i < thread_pool_size_; ++i)
            //     m_thread_pool.emplace_back([&] { m_context.run(); });

            // sigset_t wait_mask;
            // sigemptyset(&wait_mask);
            // sigaddset(&wait_mask, SIGINT);
            // sigaddset(&wait_mask, SIGQUIT);
            // sigaddset(&wait_mask, SIGTERM);
            // sigaddset(&wait_mask, SIGHUP);
            // pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
            // int signal = 0;
            // sigwait(&wait_mask, &signal);

            //m_logger->get_system_logger()->info("The system received signal {} - {}", signal, strsignal(signal));

            //m_context.stop();
            // for (auto &thread: m_thread_pool)
            // {
            //     if (thread.joinable()) thread.join();
            // }
        } catch (std::exception &e)
        {
            m_logger->get_system_logger()->error("Exception in server runner: {}", e.what());
        }
    }

    ~runner()
    {
        m_logger->get_system_logger()->info("The server is shutting down");
    }

private:
    asio::io_context m_context{ 1 };
    std::vector<std::thread> m_thread_pool{};

    int8_t thread_pool_size_ = 8;

    std::shared_ptr<LambdaSnail::logging::logger> m_logger;
};
