module;

#include <future>
#include <thread>

#include "boost/unordered/concurrent_flat_map.hpp"

export module server: resp.commands; // Move to resp module?

import resp;

namespace LambdaSnail::server
{
    export class command_dispatch
    {
    public:
        [[nodiscard]] std::future<std::string> process_command(resp::data_view message);

    private:
        //boost::concurrent_flat_map<std::string, int> m_store{};
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

std::future<std::string> LambdaSnail::server::command_dispatch::process_command(resp::data_view message)
{
    auto request = message.materialize(LambdaSnail::resp::Array{});

    // Ugly hard coding
    std::string response;
    if(request.size() == 1 and request[0].type == LambdaSnail::resp::data_type::BulkString)
    {
        auto _1 = request[0].materialize(LambdaSnail::resp::BulkString{});
        if(_1 == "PING")
        {
            LambdaSnail::server::ping_handler cmd;
            response = cmd.handle().value;
        }
    }
    else if(request.size() == 2)
    {
        auto _1 = request[0].materialize(LambdaSnail::resp::BulkString{});
        if(_1 == "ECHO")
        {
            LambdaSnail::server::echo_handler cmd;
            auto const _2 = cmd.handle(request[1]).materialize(LambdaSnail::resp::BulkString{});
            response = _2;

            // Hack to produce bulk string
            response = "$" + std::to_string(response.size()) + "\r\n" + response;
            //response = "+" + response;
        }
    }

    response += "\r\n";

    return std::async(std::launch::async, [response]{ return response; });
}

LambdaSnail::resp::data_view LambdaSnail::server::ping_handler::handle()
{
    return resp::data_view("+PONG\r\n");
}

LambdaSnail::resp::data_view LambdaSnail::server::echo_handler::handle(resp::data_view const& message)
{
    return message;
}

