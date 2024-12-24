module;

#include <future>
#include <map>
#include <shared_mutex>
#include <thread>

#include "boost/unordered/concurrent_flat_map.hpp"
#include "boost/unordered/unordered_flat_map.hpp"

export module server: resp.commands; // Move to resp module?

import resp;

namespace LambdaSnail::server
{
    export class command_dispatch
    {
    public:
        [[nodiscard]] std::future<std::string> process_command(resp::data_view message);

    private:
        //boost::concurrent_flat_map<int, int> m_store{};
        //boost::unordered_flat_map<int, int> m;
        mutable std::shared_mutex mutex{};
        std::map<std::string, std::string> m_store{};
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
        else if(_1 == "GET")
        {
            auto lock = std::shared_lock{mutex};
            auto const key = request[1].materialize(resp::BulkString{});
            auto const it = m_store.find(std::string(key.begin(), key.end()));
            if(it != m_store.end())
            {
                response = it->second;
            }
            else
            {
                response = "_\r\n";
            }
        }
    }
    else if(request.size() == 3)
    {
        auto _1 = request[0].materialize(resp::BulkString{});
        if(_1 == "SET")
        {
            auto lock = std::unique_lock{mutex};
            auto const key = request[1].materialize(resp::BulkString{});
            auto const value = request[2].value; //request[2].materialize(resp::BulkString{});
            m_store.emplace(std::string(key.begin(), key.end()), std::string(value.begin(), value.end()));
            response = "+OK\r\n";
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

