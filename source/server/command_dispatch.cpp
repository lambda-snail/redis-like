module;

#include <functional>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <tracy/Tracy.hpp>

module server;

namespace LambdaSnail::server
{
    auto command_dispatch::s_command_map = std::unordered_map<std::string_view, std::function<ICommandHandler*()>>
    {
        // std::pair("PING",std::function( [](){ return new ping_handler; })),
        // std::pair("ECHO",std::function( [](){ return new echo_handler; })),
        // std::pair("GET", std::function([&](){ return new get_handler<database>{ *m_server.get_database(m_current_db) }; })),
        // std::pair("SET", std::function([&](){ return new set_handler<database>{ *m_server.get_database(m_current_db) }; }))
    };

    command_dispatch::command_dispatch(server &server) : m_server(server)
    {


    }

    command_handler command_dispatch::get_command(std::string_view command_name) const
    {
        command_handler handler{};

        // TODO: Find a nicer way to handle this
        switch (command_name[0])
        {
            case 'P': // "PING"
                handler.create<ping_handler>();
                break;
            case 'E': // "ECHO"
                handler.create<echo_handler>();
                break;
            case 'G': // "GET"
                handler.create<get_handler<database>>(m_server.get_database(m_current_db));
                break;
            case 'S': //"SET"
                handler.create<set_handler<database>>(m_server.get_database(m_current_db));
                break;
            default:
                handler.create<static_response_handler>("Unknown command: " + std::string(command_name));
        }

        return handler;
    }

    std::string command_dispatch::process_command(resp::data_view message)
    {
        ZoneNamed(ProcessCommand, true);

        auto const request = message.materialize(resp::Array{});

        if (request.size() == 0 or request[0].type != LambdaSnail::resp::data_type::BulkString)
        {
            return {"-Unable to parse request\r\n"};
        }

        auto const command_name = request[0].materialize(resp::BulkString{});

        auto command = get_command(command_name);
        return command.execute(request);

        return {"-Unable to find command\r\n"};
    }

    void command_dispatch::set_database(server::database_handle_t index)
    {
        m_current_db = index;
    }
};