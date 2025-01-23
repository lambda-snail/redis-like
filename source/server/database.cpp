module;

#include <future>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include "libcuckoo/cuckoohash_map.hh"

#include <tracy/Tracy.hpp>

export module server: resp.commands; // Move to resp module?

import memory;
import resp;

namespace LambdaSnail::server
{
    using store_t = libcuckoo::cuckoohash_map<std::string_view, std::string>;

    struct ICommandHandler
    {
        [[nodiscard]] virtual std::string execute(std::vector<resp::data_view> const& args) const noexcept = 0;
        virtual ~ICommandHandler() = default;
    };

    struct ping_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) const noexcept override;
        ~ping_handler() override = default;
    };

    struct echo_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) const noexcept override;
        ~echo_handler() override = default;
    };

    struct get_handler final : public ICommandHandler
    {
        explicit get_handler(store_t const& store) : m_store(store) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) const noexcept override;
        ~get_handler() override = default;
    private:
        store_t const& m_store;
    };

    struct set_handler final : public ICommandHandler
    {
        explicit set_handler(store_t& store) : m_store(store) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) const noexcept override;
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

        // std::unordered_map<std::string_view, ICommandHandler const*> m_command_map
        // {
        //     { "PING", new ping_handler },
        //     { "ECHO", new echo_handler },
        //     { "GET", new get_handler{ m_store } },
        //     { "SET", new set_handler{ m_store } }
        // };

        std::unordered_map<std::string_view, std::function<std::shared_ptr<ICommandHandler>()>> m_command_map;


        //std::shared_ptr<ICommandHandler> get_command(std::string_view command) const;

        std::shared_mutex m_mutex{};
    };
}

std::string LambdaSnail::server::database::process_command(resp::data_view message)
{
    ZoneNamed(ProcessCommand, true);

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

     // if (cmd_it != m_command_map.end())
     // {
     //     auto const* cmd = cmd_it->second;
     //     if (cmd)
     //     {
     //         std::string s = cmd->execute(request);
     //         return s;
     //     }
     // }

    return { "-Unable to find command\r\n" };
}

std::string LambdaSnail::server::ping_handler::execute(std::vector<resp::data_view> const& args) const noexcept
{
    ZoneScoped;

    assert(args.size() == 1);
    return "+PONG\r\n";
}

std::string LambdaSnail::server::echo_handler::execute(std::vector<resp::data_view> const& args) const noexcept
{
    ZoneScoped;

    assert(args.size() == 2);
    auto const str = args[1].materialize(resp::BulkString{});
    return "$" + std::to_string(str.size()) + "\r\n" + std::string(str.data(), str.size()) + "\r\n";
}

std::string LambdaSnail::server::get_handler::execute(std::vector<resp::data_view> const& args) const noexcept
{
    ZoneScoped;

    if (args.size() == 2)
    {
        auto const key = args[1].materialize(resp::BulkString{});
        if(std::string value; m_store.find(key, value))
        {
            return value + "\r\n";
        }
    }

    return "_\r\n";
}

std::string LambdaSnail::server::set_handler::execute(std::vector<resp::data_view> const &args) const noexcept
{
    ZoneScoped;

    if (args.size() == 3)
    {
        auto const key = args[1].materialize(resp::BulkString{});
        auto const value = args[2].value; //request[2].materialize(resp::BulkString{});
        //m_store.emplace(std::string(key.begin(), key.end()), std::string(value.begin(), value.end()));
        m_store.insert_or_assign(key, value);
        return "+OK\r\n";
    }

    return "-Unable to SET\r\n";
}

