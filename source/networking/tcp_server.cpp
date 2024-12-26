module;

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ts/internet.hpp>

#include <iostream>

export module networking :resp.tcp_server;

import memory;
import server;
import resp;

template<typename Allocator>
class tcp_connection : public std::enable_shared_from_this<tcp_connection<Allocator>>
{
public:
    typedef std::shared_ptr<tcp_connection> connection_ptr;

    static connection_ptr create(asio::io_context &io_context, LambdaSnail::server::command_dispatch& dispatch, Allocator& allocator)
    {
        return connection_ptr(new tcp_connection(io_context, dispatch, allocator));
    }

    asio::ip::tcp::socket& get_socket()
    {
        return m_socket;
    }

    void read_request()
    {
        m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()),
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
        m_allocator.deallocate(&m_buffer, m_buffer.size());
        //std::clog << "[Connection] Destroyed" << std::endl;
    }
private:
    void handle_command(size_t length)
    {
        LambdaSnail::resp::data_view resp_data(std::string_view(m_buffer.begin(), m_buffer.end()));

        std::future<std::string> response_f = m_dispatch.process_command(resp_data);
        response_f.wait();
        auto response = response_f.get();

        asio::async_write(m_socket, asio::buffer(response),
          [this_ = this->shared_from_this()](asio::error_code ec, size_t length)
              {
                  this_->handle_write(ec, length);
              });
    }

    explicit tcp_connection(asio::io_context &io_context, LambdaSnail::server::command_dispatch& dispatch, Allocator& allocator) : m_dispatch(dispatch), m_socket(io_context), m_allocator(allocator), m_buffer{ *allocator.allocate(1024) }
    {
        //std::array<char, 1024>& buffer
        // TODO: Hard-coded for now
        //std::array<char, 1024ul>* m_buffer = allocator.allocate(1024); // std::allocator_traits<Allocator>::allocate(allocator, 1024);
        //m_buffer = *allocator.allocate(1024); // std::allocator_traits<Allocator>::allocate(allocator, 1024);
    }

    void handle_write(std::error_code const &ec, size_t bytes)
    {
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

    Allocator& m_allocator;
    LambdaSnail::server::command_dispatch& m_dispatch;
    asio::ip::tcp::socket m_socket;

    //std::array<char, 10 * 1024> m_buffer{};
    std::array<char, 1024>& m_buffer;
};

template<typename Allocator = std::allocator<void>>
class tcp_server
{
public:
    explicit tcp_server(asio::io_context& context, uint16_t const port, LambdaSnail::server::command_dispatch& dispatch)
        : m_dispatch(dispatch), m_asio_context(context), m_acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        typename tcp_connection<Allocator>::connection_ptr new_connection =
                tcp_connection<Allocator>::create(m_asio_context, m_dispatch, allocator);

        m_acceptor.async_accept(new_connection->get_socket(),
            [=, this](asio::error_code ec)
            {
               handle_accept(new_connection, ec);
            });
    }

    void handle_accept(tcp_connection<Allocator>::connection_ptr const &new_connection, std::error_code const &ec)
    {
        if (not ec)
        {
            new_connection->read_request();
        }
        else
        {
            std::cerr << "[Server] Error when establishing connection: " << ec.message() << std::endl;
        }

        start_accept();
    }

    LambdaSnail::server::command_dispatch& m_dispatch;
    //typename std::allocator_traits<Allocator>::allocator_type allocator{};
    Allocator allocator{};

    asio::io_context& m_asio_context;
    asio::ip::tcp::acceptor m_acceptor;
};

/**
 * Extracts the value of an environment variable with a numeric value.
 * @var_string The string of the variable, including the '=' character.
 */
template<typename value_t>
static constexpr bool get_environment_value(std::string_view var_string, char** env_vars, value_t& value)
{
    for(int32_t i = 0; env_vars[i] != nullptr; ++i)
    {
        std::string_view env_var(env_vars[i]);

        if(env_var.starts_with(var_string))
        {
            auto value_str = env_var.substr(var_string.length(), env_var.length());
            if(not value_str.empty())
            {
                if(std::from_chars(env_var.data() + var_string.length(), env_var.data() + env_var.length(), value).ec == std::errc{})
                {
                    return true;
                }
            }
        }
    }

    value = {};
    return false;
};

// TODO: Move this out from here
export template<typename Allocator = std::allocator<void>>
class runner
{
public:
    void run(uint16_t port, LambdaSnail::server::command_dispatch& dispatch)
    {
        try
        {
            asio::io_context context;
            tcp_server<Allocator> server(context, port, dispatch);
            context.run();
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
};