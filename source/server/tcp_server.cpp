module;

#include <iostream>
#include <ranges>
#include <string_view>

#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ts/internet.hpp>

export module networking :resp.server;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> connection_ptr;

    static connection_ptr create(asio::io_context &io_context)
    {
        return connection_ptr(new tcp_connection(io_context));
    }

    asio::ip::tcp::socket& get_socket()
    {
        return m_socket;
    }

    void read_request()
    {
        m_socket.async_read_some(asio::buffer(m_buffer.data(), m_buffer.size()),
        [this_ = shared_from_this()](std::error_code const e, size_t length)
        {
            if (not e)
            {
                std::cout << std::endl << std::format(
                    "[Connection] Bytes available for reading: {}", length) << std::endl;

                this_->m_data.insert(this_->m_data.begin(), this_->m_buffer.begin(), this_->m_buffer.end());
                // for (size_t b = 0; b < length; ++b)
                // {
                //     this_->m_data[b] = this_->m_buffer[b];
                //     // if (this_->m_buffer[b] != '\r')
                //     // {
                //     //     std::clog << this_->m_buffer[b];
                //     // }
                // }

                // auto it_end = this_->m_buffer.begin();
                // std::ranges::advance(it_end, static_cast<std::iter_difference_t<decltype(this_->m_buffer)>>(length));
                //
                // this_->m_data.insert(this_->m_data.end(), this_->m_buffer.begin(), this_->m_buffer.end());

                // Need to parse http header to find content length and continue reading rest of the data
                // Or better yet, use library for handling http related stuff
                // this_->read_request();

                // Since this is just a demo, we stop reading here and write the response
                this_->write_echo();
            } else if (e != asio::error::eof)
            {
                std::clog << std::format("Handler encountered error: {}", e.message());
            } else
            {
                this_->write_echo();
            }
        });
    }

    ~tcp_connection()
    {
        std::clog << "[Connection] Destroyed" << std::endl;
    }
private:
    void write_echo()
    {
        std::clog << "[Connection] --- Response ---" << std::endl;
        for (auto c : m_data)
        {
            if (c != '\r')
            {
                std::clog << c;
            }
        }
        std::clog << "[Connection] --- End Response ---" << std::endl;

        asio::async_write(m_socket, asio::buffer(m_data),
          [this_ = shared_from_this()](asio::error_code ec, size_t length)
              {
                  this_->handle_write(ec, length);
              });
    }

    explicit tcp_connection(asio::io_context &io_context) : m_socket(io_context) { }

    void handle_write(std::error_code const &ec, size_t bytes)
    {
        if (not ec)
        {
            std::clog << "[Connection] Wrote " << bytes << " bytes" << std::endl;
            //read_request();
        }
        else if (ec != asio::error::eof)
        {
            std::clog << "[Connection] Error when establishing connection: " << ec.message() << std::endl;
        }
    }

    asio::ip::tcp::socket m_socket;

    std::vector<char> m_data{};
    std::array<char, 10 * 1024> m_buffer{};
};

class tcp_server
{
public:
    explicit tcp_server(asio::io_context& context, uint16_t const port)
        : m_asio_context(context), m_acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        tcp_connection::connection_ptr new_connection =
                tcp_connection::create(m_asio_context);

        m_acceptor.async_accept(new_connection->get_socket(),
            [=, this](asio::error_code ec)
            {
               handle_accept(new_connection, ec);
            });
    }

    void handle_accept(tcp_connection::connection_ptr const &new_connection, std::error_code const &ec)
    {
        if (!ec)
        {
            new_connection->read_request();
        } else
        {
            std::clog << "[Server] Error when establishing connection: " << ec.message() << std::endl;
        }

        start_accept();
    }

    asio::io_context& m_asio_context;
    asio::ip::tcp::acceptor m_acceptor;
};

static constexpr std::string_view custom_handler_port_str = "FUNCTIONS_CUSTOMHANDLER_PORT=";

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
export class runner
{
public:
    void run(uint16_t port)
    {
        try
        {
            asio::io_context context;
            tcp_server server(context, port);
            context.run();
        } catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
};