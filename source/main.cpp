module;

#include <iostream>
#include <string>

export module main;

import memory;
import networking;
import resp;
import server;

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
    using ls_string = std::basic_string<char, std::char_traits<char>, LambdaSnail::memory::buffer_allocator<char>>;

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    ls_string str(allocator);
    str = "saaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    // LambdaSnail::server::command_dispatch dispatch{allocator};
    //
    // runner runner;
    // runner.run(6379, dispatch, buffer_pool);

    return 0;
}
