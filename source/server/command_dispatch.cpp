module;

#include <string_view>
#include <unordered_map>

#include <tracy/Tracy.hpp>

module server;

import resp;

namespace LambdaSnail::server
{
    auto command_dispatch::s_command_map = std::unordered_map<std::string_view, ICommandHandler* const>
    {
        std::pair("PING", new ping_handler),
        std::pair("ECHO", new echo_handler),
        std::pair("GET", new get_handler<database>{ *this }),
        std::pair("SET", new set_handler<database>{ *this })
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
        auto const cmd_it = m_command_map.find(command_name);
        if (cmd_it != m_command_map.end())
        {
            auto *const command = cmd_it->second;
            if (command)
            {
                std::string s = command->execute(request);
                return s;
            }
        }

        return {"-Unable to find command\r\n"};
    }

    void command_dispatch::set_database(server::database_handle_t index)
    {
        m_current_db = index;
    }
};