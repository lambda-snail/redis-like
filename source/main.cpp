import networking;
import resp;

#include <iostream>
#include <string>

void parse_and_print(LambdaSnail::resp::parser const& parser, std::string const& s)
{
    auto result = parser.parse_message(s);
    std::cout << static_cast<int32_t>(result.type) << std::endl;
    std::cout << result.value << std::endl;
}

int main()
{
    // runner runner;
    // runner.run(6379);

    //char const* message = "$4\r\nINCR\r\n";
    //auto message = "*2\r\n$4\r\nINCR\r\n$4\r\nINCR\r\n";
    auto message = "*2\r\n$5\r\nHello\r\n$2\r\nThe World\r\n";

    LambdaSnail::resp::parser p;
    //auto result = p.parse_message(message);
    parse_and_print(p, message);

    return 0;
}
