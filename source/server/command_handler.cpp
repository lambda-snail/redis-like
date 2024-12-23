module;

import resp;

export module server: resp.commands; // Move to resp module?

namespace LambdaSnail::server
{
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

