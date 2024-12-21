module;

#include <iostream>
#include <string>

export module main;

import networking;
import resp;

void materialize_and_print_array(std::string_view msg)
{
    LambdaSnail::resp::data_view data(msg);
    auto const values = data.materialize(LambdaSnail::resp::Array{});
    for(auto const& v : values)
    {
        std::cout << static_cast<char>(v.type) << " - " << v.value <<std::endl;
    }
}

template <typename T>
void materialize_and_print(std::string_view msg)
{
    LambdaSnail::resp::data_view data(msg);
    auto const value = data.materialize(T{});
    std::cout << value << std::endl;
}

void materialize_and_print(std::string_view msg)
{
    LambdaSnail::resp::data_view data(msg);
    auto const value = data.is_null();
    std::cout << "is null: " << value << std::endl;
}

int main()
{
    // LambdaSnail::resp::data_view data("#t\r\n");
    // auto const value = data.materialize(LambdaSnail::resp::Boolean{});
    // std::cout << value << std::endl;
    //
    // materialize_and_print<LambdaSnail::resp::Integer>(":12345\r\n");
    // materialize_and_print<LambdaSnail::resp::Integer>(":-12345\r\n");
    //
    // materialize_and_print<LambdaSnail::resp::Double>(",12.34\r\n");
    // materialize_and_print<LambdaSnail::resp::Double>(",-12.34\r\n");
    //
    // materialize_and_print("_\r\n");
    //materialize_and_print_array("*2\r\n$4\r\nINCR\r\n+INCR\r\n");

    // materialize_and_print<LambdaSnail::resp::BulkString>("$4\r\nINCR\r\n");

    runner runner;
    runner.run(6379);

    return 0;
}
