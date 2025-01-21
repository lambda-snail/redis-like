module;

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ts/internet.hpp>

#include <exception>
#include <iostream>
#include <thread>

#include <signal.h>

#include <tracy/Tracy.hpp>

export module networking :resp.tcp_server;

import logging;
import memory;
import server;
import resp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> connection_ptr;

    static connection_ptr create(asio::io_context &io_context, LambdaSnail::server::database& dispatch, LambdaSnail::memory::buffer_pool& buffer_pool)
    {
        return connection_ptr(new tcp_connection(io_context, dispatch, buffer_pool));
    }

    asio::ip::tcp::socket& get_socket()
    {
        return m_socket;
    }

    void read_request()
    {
        ZoneScoped;

        m_socket.async_read_some(asio::buffer(m_buffer.buffer, m_buffer.size),
        [this_ = this->shared_from_this()](std::error_code const e, size_t length)
        {
            if (not e)
            {
                // std::cout << std::endl << std::format(
                //     "[Connection] Bytes available for reading: {}", length) << std::endl;

                // this_->m_data.insert(this_->m_data.begin(), this_->m_buffer.begin(), this_->m_buffer.end());

                this_->handle_command(length);
            } else if (e != asio::error::eof)
            {
                std::cerr << std::format("Handler encountered error: {}", e.message());
            } else
            {
                //this_->handle_command();
            }
        });
    }

    ~tcp_connection()
    {
        m_buffer_pool.release_buffer(m_buffer.buffer);
        //m_allocator.deallocate(&m_buffer, m_buffer.size());
        //std::clog << "[Connection] Destroyed" << std::endl;
    }
private:
    void handle_command(size_t length)
    {
        ZoneScoped;

        LambdaSnail::resp::data_view resp_data(std::string_view(m_buffer.buffer, m_buffer.size));
        std::string response = m_dispatch.process_command(resp_data);

        asio::async_write(m_socket, asio::buffer(response),
          [this_ = this->shared_from_this()](asio::error_code ec, size_t length)
              {
                  this_->handle_write(ec, length);
              });
    }

    explicit tcp_connection(asio::io_context &io_context, LambdaSnail::server::database& dispatch, LambdaSnail::memory::buffer_pool& buffer_pool) : m_dispatch(dispatch), m_socket(io_context), m_buffer_pool(buffer_pool), m_buffer{}
    {
        //std::array<char, 1024>& buffer
        // TODO: Hard-coded for now
        //std::array<char, 1024ul>* m_buffer = allocator.allocate(1024); // std::allocator_traits<Allocator>::allocate(allocator, 1024);
        //m_buffer = *allocator.allocate(1024); // std::allocator_traits<Allocator>::allocate(allocator, 1024);
        m_buffer = m_buffer_pool.request_buffer(1024);
    }

    void handle_write(std::error_code const &ec, size_t bytes)
    {
        ZoneScoped;

        if (not ec)
        {
            // std::clog << "[Connection] Wrote " << bytes << " bytes" << std::endl;
            //read_request();
        }
        else if (ec != asio::error::eof)
        {
            std::cerr << "[Connection] Error when establishing connection: " << ec.message() << std::endl;
        }
    }

    LambdaSnail::memory::buffer_pool& m_buffer_pool;
    LambdaSnail::server::database& m_dispatch;
    asio::ip::tcp::socket m_socket;

    //std::array<char, 10 * 1024> m_buffer{};
    //std::array<char, 1024>& m_buffer;
    LambdaSnail::memory::buffer_info m_buffer;
};

class tcp_server
{
public:
    explicit tcp_server(asio::io_context& context, uint16_t const port, LambdaSnail::server::database& dispatch, LambdaSnail::memory::buffer_pool& buffer_pool)
        :   m_dispatch(dispatch),
            m_buffers(buffer_pool),
            m_asio_context(context),
            m_acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        ZoneScoped;

        typename tcp_connection::connection_ptr new_connection =
                tcp_connection::create(m_asio_context, m_dispatch, m_buffers);

        m_acceptor.async_accept(new_connection->get_socket(),
            [=, this](asio::error_code ec)
            {
               handle_accept(new_connection, ec);
            });
    }

    void handle_accept(tcp_connection::connection_ptr const &new_connection, std::error_code const &ec)
    {
        ZoneScoped;

        if (not ec)
        {
            new_connection->read_request();
        }
        else //if (ec == asio::error::operation_aborted) should maybe not log an error when operation aborted
        {
            std::cerr << "[Server] Error when establishing connection: " << ec.message() << std::endl;
        }

        start_accept();
    }

    LambdaSnail::server::database& m_dispatch;
    //typename std::allocator_traits<Allocator>::allocator_type allocator{};
    LambdaSnail::memory::buffer_pool& m_buffers;

    asio::io_context& m_asio_context;
    asio::ip::tcp::acceptor m_acceptor;
};

// TODO: Move this out from here
export class runner
{
public:
    runner(std::shared_ptr<LambdaSnail::logging::logger> logger) : m_logger(logger) {  }

    void run(uint16_t port, LambdaSnail::server::database& dispatch, LambdaSnail::memory::buffer_pool& buffer_pool)
    {
        ZoneScoped;

        try
        {
            tcp_server server(m_context, port, dispatch, buffer_pool);

            // See https://think-async.com/Asio/asio-1.30.2/src/examples/cpp11/http/server3/server.cpp
            for (std::size_t i = 0; i < thread_pool_size_; ++i)
                m_thread_pool.emplace_back([&]{ m_context.run(); });

            sigset_t wait_mask;
            sigemptyset(&wait_mask);
            sigaddset(&wait_mask, SIGINT);
            sigaddset(&wait_mask, SIGQUIT);
            sigaddset(&wait_mask, SIGTERM);
            sigaddset(&wait_mask, SIGHUP);
            pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
            int signal = 0;
            sigwait(&wait_mask, &signal);

            m_logger->get_system_logger()->info("The system received signal {} - {}", signal, strsignal(signal));

            m_context.stop();
            for (auto& thread : m_thread_pool)
            {
                if (thread.joinable()) thread.join();
            }
        }
        catch (std::exception &e)
        {
            m_logger->get_system_logger()->error("Exception in server runner: {}", e.what());
        }
    }

    ~runner()
    {
        m_logger->get_system_logger()->info("The server is shutting down");
    }
private:
    asio::io_context m_context{};
    std::vector<std::thread> m_thread_pool{};

    int8_t thread_pool_size_ = 8;

    std::shared_ptr<LambdaSnail::logging::logger> m_logger;
};