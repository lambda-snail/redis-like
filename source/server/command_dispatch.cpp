module;

#include <future>

#include "boost/unordered/concurrent_flat_map.hpp"

export module server: resp.commands; // Move to resp module?

import resp;

namespace LambdaSnail::server
{
    export class command_dispatch
    {
        [[nodiscard]] std::future<std::string> process_command();

    private:
        boost::concurrent_flat_map<std::string, int> m_store{};
    };



    export struct ping_handler
    {
        resp::data_view handle();
    };

    export struct echo_handler
    {
        resp::data_view handle(resp::data_view const& message);
    };
}

LambdaSnail::resp::data_view LambdaSnail::server::ping_handler::handle()
{
    return resp::data_view("+PONG\r\n");
}

LambdaSnail::resp::data_view LambdaSnail::server::echo_handler::handle(resp::data_view const& message)
{
    return message;
}

