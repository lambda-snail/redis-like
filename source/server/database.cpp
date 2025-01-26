module;

#include <atomic>
#include <cassert>
#include <functional>
#include <future>
#include <shared_mutex>
#include <string>
#include <charconv>
#include <iomanip>

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <tracy/Tracy.hpp>

export module server: database; // Move to resp module?

import memory;
import resp;

namespace LambdaSnail::server
{
    export struct entry_info
    {
        typedef uint32_t version_t;

        version_t version{};
        std::string data;
        std::chrono::time_point<std::chrono::system_clock> ttl{};

        [[nodiscard]] bool has_timeout() const
        {
            return ttl != std::chrono::time_point<std::chrono::system_clock>::min();
        }
    };

    //using store_t = std::unordered_map<std::string, std::shared_ptr<struct value_info>>; // TODO: Should be string!!! ?
    using store_t = tbb::concurrent_unordered_map<std::string, std::shared_ptr<entry_info>>;

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

    template<typename TDatabase>
    struct get_handler final : public ICommandHandler
    {
        explicit get_handler(TDatabase& database) : m_database(database) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~get_handler() override = default;
    private:
        TDatabase& m_database;
    };

    template<typename TDatabase>
    struct set_handler final : public ICommandHandler
    {
        explicit set_handler(TDatabase& database) : m_database(database) {}
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const& args) noexcept override;
        ~set_handler() override = default;
    private:
        TDatabase& m_database;
    };

    export class database
    {
    public:
        explicit database(memory::buffer_allocator<char>& string_allocator) : m_string_allocator(string_allocator) { }
        [[nodiscard]] std::string process_command(resp::data_view message);

        [[nodiscard]] std::shared_ptr<entry_info> get_value(std::string const& key);

        void set_value(std::string const& key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl = {});

    private:
        memory::buffer_allocator<char>& m_string_allocator;

        store_t m_store{1000};

        // TODO: May need different structure for this when we can support multiple databases
        tbb::concurrent_unordered_map<std::string_view, ICommandHandler* const> m_command_map
        {
            std::pair("PING", new ping_handler),
            std::pair("ECHO", new echo_handler),
            std::pair("GET", new get_handler<database>{ *this }),
            std::pair("SET", new set_handler<database>{ *this }),
        };

        /**
         * The key-value store is a concurrent queue, so is "safe" to access from multiple threads,
         * but we may periodically wish to perform maintenance work on the database, such as expire
         * key etc. In those cases the shared mutex allows us to lock the entire map for a short
         * duration.
         */
        mutable std::shared_mutex m_mutex{};
    };
}

std::string LambdaSnail::server::database::process_command(resp::data_view message)
{
    ZoneNamed(ProcessCommand, true);

    auto const request = message.materialize(resp::Array{});

    if (request.size() == 0 or request[0].type != LambdaSnail::resp::data_type::BulkString)
    {
        return { "-Unable to parse request\r\n" };
    }

    auto const _1 = request[0].materialize(resp::BulkString{});
    auto const cmd_it = m_command_map.find(_1);
    if (cmd_it != m_command_map.end())
    {
        auto* const command = cmd_it->second;
        if (command)
        {
            std::string s = command->execute(request);
            return s;
        }
    }

    return { "-Unable to find command\r\n" };
}

std::shared_ptr<LambdaSnail::server::entry_info> LambdaSnail::server::database::get_value(std::string const &key)
{
    auto lock = std::shared_lock{ m_mutex };

    auto const it = m_store.find(key);
    if (it == m_store.end())
    {
        return nullptr;
    }

    if (it->second->has_timeout())
    {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        if (it->second->ttl < now)
        {
            m_store.unsafe_erase(it);
            return nullptr;
        }
    }

    return it->second;
}

void LambdaSnail::server::database::set_value(std::string const &key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl)
{
    auto lock = std::shared_lock{ m_mutex };

    auto const it = m_store.find(key);
    std::shared_ptr<entry_info> const value_wrapper = (it == m_store.end()) ? std::make_shared<entry_info>() : it->second;

    value_wrapper->data = std::string(value);
    ++value_wrapper->version;
    value_wrapper->ttl = ttl;

    m_store[key] = value_wrapper;
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

template<typename TDatabase>
std::string LambdaSnail::server::get_handler<TDatabase>::execute(std::vector<resp::data_view> const& args) noexcept
{
    ZoneScoped;

    if (args.size() == 2)
    {
        auto const key = std::string(args[1].materialize(resp::BulkString{}));

        auto value = m_database.get_value(std::move(key));
        if (value)
        {
            return value->data + "\r\n";
        }
    }

    return "_\r\n";
}

template<typename TDatabase>
std::string LambdaSnail::server::set_handler<TDatabase>::execute(std::vector<resp::data_view> const &args) noexcept
{
    ZoneScoped;

    if (args.size() == 3)
    {
        auto const key = std::string(args[1].materialize(resp::BulkString{}));
        auto value = args[2].value;
        m_database.set_value(key, value);
        return "+OK\r\n";
    }

    if (args.size() == 5)
    {
        auto const key = std::string(args[1].materialize(resp::BulkString{}));
        auto value = args[2].value;
        auto option = args[3].value; // Assume EX or PX for now

        // Redis CLI sends a bulk string - need to refactor parsing part to handle various cases
        auto ttl_str = args[4].materialize(resp::BulkString{});

        //auto conversion = std::to_integer(ttl_str);
        int64_t ttl{};
        std::from_chars(ttl_str.data(), ttl_str.data() + ttl_str.length(), ttl);
        if (ttl == 0) [[unlikely]]
        {
            return "-Invalid option to SET command, EX and PX require a non-negative integer\r\n";
        }

        if (option == "PX")
        {
            m_database.set_value(key, value, std::chrono::system_clock::now() + std::chrono::seconds(ttl));
        }
        else
        {
            m_database.set_value(key, value, std::chrono::system_clock::now() + std::chrono::milliseconds(ttl));
        }

        return "+OK\r\n";
    }


    return "-Unable to SET\r\n";
}

