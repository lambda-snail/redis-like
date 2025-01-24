module;

#include <cassert>
#include <functional>
#include <future>
#include <shared_mutex>
#include <unordered_map>

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <tracy/Tracy.hpp>

export module server: resp.commands; // Move to resp module?

import memory;
import resp;

namespace LambdaSnail::server
{
    //using store_t = std::unordered_map<std::string, std::string>; // TODO: Should be string!!! ?
    using store_t = tbb::concurrent_unordered_map<std::string, std::string>;

    struct ICommandHandler
    {
        [[nodiscard]] virtual std::string execute(std::vector<resp::data_view> const& args) noexcept = 0;
        virtual ~ICommandHandler() = default;
    };

    struct ping_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~ping_handler() override = default;
    };

    struct echo_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~echo_handler() override = default;
    };

    struct get_handler final : public ICommandHandler
    {
        explicit get_handler(store_t& store) : m_store(store) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~get_handler() override = default;
    private:
        store_t& m_store;
    };

    struct set_handler final : public ICommandHandler
    {
        explicit set_handler(store_t& store) : m_store(store) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~set_handler() override = default;
    private:
        store_t& m_store;
    };

    export class database
    {
    public:
        explicit database(memory::buffer_allocator<char>& string_allocator) : m_string_allocator(string_allocator)
        {
            m_command_map.insert_or_assign("PING", [](){ return std::make_shared<ping_handler>(); });
            m_command_map.insert_or_assign("ECHO", [](){ return std::make_shared<echo_handler>(); });
            m_command_map.insert_or_assign("GET", [&store=m_store](){ return std::make_shared<get_handler>(store); });
            m_command_map.insert_or_assign("SET", [&store=m_store](){ return std::make_shared<set_handler>(store); });
        }
        [[nodiscard]] std::string process_command(resp::data_view message);

    private:
        memory::buffer_allocator<char>& m_string_allocator;

        store_t m_store{1000};

        std::unordered_map<std::string_view, std::function<std::shared_ptr<ICommandHandler>()>> m_command_map;

        std::shared_mutex m_mutex{};
    };
}

std::string LambdaSnail::server::database::process_command(resp::data_view message)
{
    ZoneNamed(ProcessCommand, true);

    // TODO: Think about: No need to lock as it's single-threaded? But doesn't seem to impact performance
    //auto lock = std::unique_lock { m_mutex };

    auto const request = message.materialize(resp::Array{});

    if (request.size() == 0 or request[0].type != LambdaSnail::resp::data_type::BulkString)
    {
        return { "-Unable to parse request\r\n" };
    }

    auto const _1 = request[0].materialize(resp::BulkString{});
    // get_handler cmd(m_store);
    // return cmd.execute(request);

    auto const cmd_it = m_command_map.find(_1);
    if (cmd_it != m_command_map.end())
    {
        auto const& factory_fn = cmd_it->second;
        auto const command = factory_fn();
        if (command)
        {
            std::string s = command->execute(request);
            return s;
        }
    }

    return { "-Unable to find command\r\n" };
}

std::string LambdaSnail::server::ping_handler::execute(std::vector<resp::data_view> const& args) noexcept
{
    ZoneScoped;

    assert(args.size() == 1);
    return "+PONG\r\n";
}

std::string LambdaSnail::server::echo_handler::execute(std::vector<resp::data_view> const& args) noexcept
{
    ZoneScoped;

    assert(args.size() == 2);
    auto const str = args[1].materialize(resp::BulkString{});
    return "$" + std::to_string(str.size()) + "\r\n" + std::string(str.data(), str.size()) + "\r\n";
}

std::string LambdaSnail::server::get_handler::execute(std::vector<resp::data_view> const& args) noexcept
{
    ZoneScoped;

    if (args.size() == 2)
    {
        auto const key = std::string(args[1].materialize(resp::BulkString{}));

        auto it = m_store.find(key);
        if (it != m_store.end())
        //if(std::string value; m_store.find(key, value))
        {
            return it->second + "\r\n";
        }
    }

    return "_\r\n";
}

std::string LambdaSnail::server::set_handler::execute(std::vector<resp::data_view> const &args) noexcept
{
    ZoneScoped;

    if (args.size() == 3)
    {
        auto const key = std::string(args[1].materialize(resp::BulkString{}));
        auto const value = std::string(args[2].value); //request[2].materialize(resp::BulkString{});

        //m_store.insert_or_assign(key, value);
        m_store[key] = value;

        return "+OK\r\n";
    }

    return "-Unable to SET\r\n";
}

