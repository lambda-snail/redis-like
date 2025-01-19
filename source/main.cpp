module;

#include <iostream>
#include <string>

#include <tracy/Tracy.hpp>

export module main;

import memory;
import networking;
import resp;
import server;

int main()
{
    ZoneScoped;

    LambdaSnail::memory::buffer_pool buffer_pool{};
    LambdaSnail::memory::buffer_allocator<char> allocator{buffer_pool};

    LambdaSnail::server::database dispatch{allocator};

    runner runner;
    runner.run(6379, dispatch, buffer_pool);

    return 0;
}
