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

    std::string command_dispatch::process_command(resp::data_view message)
    {
        ZoneNamed(ProcessCommand, true);

        auto const request = message.materialize(resp::Array{});

        if (request.size() == 0 or request[0].type != LambdaSnail::resp::data_type::BulkString)
        {
            return {"-Unable to parse request\r\n"};
        }

        auto const command_name = request[0].materialize(resp::BulkString{});

        //auto const cmd_it = s_command_map.find(command_name);
        auto const command = get_command(command_name);
        return command(request);

        // if (cmd_it != s_command_map.end())
        // {
        //     auto *const command = cmd_it->second;
        //     if (command)
        //     {
        //         std::string s = command->execute(request);
        //         return s;
        //     }
        // }

        return {"-Unable to find command\r\n"};
    }

    void command_dispatch::set_database(server::database_handle_t index)
    {
        m_current_db = index;
    }

    auto
    command_dispatch::get_command(std::string_view command_name) const
    {
        std::variant<ping_handler, echo_handler, get_handler<database>, set_handler<database>> variant;

        switch (command_name)
        {
            case "PING":
                variant = ping_handler();
                break;
            case "ECHO":
                variant = echo_handler();
                break;
            case "GET":
                variant = std::move(get_handler<database>{ m_server.get_database(m_current_db) });
                break;
            case "SET":
                variant = std::move(set_handler<database>{ m_server.get_database(m_current_db) });
                break;
            default:
                variant = static_response_handler("Unknown command: " + std::string(command_name));
        }

        return [variant](std::vector<resp::data_view>& command)
        {
            return std::visit([command](auto&& handler)
            {
                return handler->exeute(command);
            },
            variant);
        };
    }
};