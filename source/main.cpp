import networking;
import resp;

#include <iostream>
#include <string>

void parse_and_print(LambdaSnail::resp::parser const& parser, std::string const& s)
{
    auto result = parser.parse_simple(s);
    std::cout << static_cast<int32_t>(result.type) << std::endl;
    std::cout << result.value << std::endl;
}

int main()
{
    runner runner;
    runner.run(6379);

    return 0;
}
