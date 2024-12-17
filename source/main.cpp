import networking;
import resp;

#include <iostream>
#include <string>

void parse_and_print(LambdaSnail::resp::parser const& parser, std::string const& s)
{
    auto result = parser.parse_message_s(s);
    std::cout << static_cast<int32_t>(result.type) << std::endl;
    std::cout << result.value << std::endl;
}

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
    std::cout << value << std::endl;
}

int main()
{
    LambdaSnail::resp::data_view data("#t\r\n");
    auto const value = data.materialize(LambdaSnail::resp::Boolean{});
    std::cout << value << std::endl;

    materialize_and_print<LambdaSnail::resp::Integer>(":12345\r\n");
    materialize_and_print<LambdaSnail::resp::Integer>(":-12345\r\n");

    materialize_and_print<LambdaSnail::resp::Double>(",12.34\r\n");
    materialize_and_print<LambdaSnail::resp::Double>(",-12.34\r\n");

    materialize_and_print("_\r\n");
    materialize_and_print_array("*2\r\n$4\r\nINCR\r\n+INCR\r\n");

    // runner runner;
    // runner.run(6379);

    //char const* message = "$4\r\nINCR\r\n";
    //auto message = "*2\r\n$4\r\nINCR\r\n$4\r\nINCR\r\n";
    //auto message = "*2\r\n$5\r\nHello\r\n$9\r\nThe World\r\n";
    //auto message = "#t\r\n";
    //auto message = "_\r\n";

    //LambdaSnail::resp::parser p;
    //auto result = p.parse_message(message);
    //parse_and_print(p, message);

    return 0;
}
