import resp;

#include <iostream>
#include <string>

void parse_and_print(LambdaSnail::resp::parser const& parser, std::string const& s)
{
    auto result = parser.parse(s);
    std::cout << static_cast<int32_t>(result.type) << std::endl;
    std::cout << result.value << std::endl;
}

int main()
{
    LambdaSnail::resp::parser parser;

    parse_and_print(parser, "+Hello World\r\n");
    parse_and_print(parser, ":12345678\r\n");
    parse_and_print(parser, ",123478\r\n");
    parse_and_print(parser, ",123.478\r\n");
    parse_and_print(parser, ",123.47xyz8\r\n");
    parse_and_print(parser, "#F\r\n");
    parse_and_print(parser, "#t\r\n");
    parse_and_print(parser, "#Xyre\r\n");
    parse_and_print(parser, "_\r\n");
    parse_and_print(parser, "_xyq\r\n");

    return 0;
}
