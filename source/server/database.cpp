module;

#include <atomic>
#include <cassert>
#include <charconv>
#include <functional>
#include <future>
#include <iomanip>
#include <random>
#include <semaphore>
#include <shared_mutex>
#include <string>

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <tracy/Tracy.hpp>

module server;

namespace LambdaSnail::server
{
    struct ping_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;

        ~ping_handler() override = default;
    };

    struct echo_handler final : public ICommandHandler
    {
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;

        ~echo_handler() override = default;
    };

    template<typename TDatabase>
    struct get_handler final : public ICommandHandler
    {
        explicit get_handler(TDatabase &database) : m_database(database)
        {
        }

        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;

        ~get_handler() override = default;

    private:
        TDatabase &m_database;
    };

    template<typename TDatabase>
    struct set_handler final : public ICommandHandler
    {
        explicit set_handler(TDatabase &database) : m_database(database)
        {
        }

        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;

        ~set_handler() override = default;

    private:
        TDatabase &m_database;
    };

    bool entry_info::has_ttl() const
    {
        return ttl != std::chrono::time_point<std::chrono::system_clock>::min();
    }

    bool entry_info::has_expired(time_point_t now) const
    {
        return ttl <= now;
    }

    database::database(memory::buffer_allocator<char> &string_allocator) : m_string_allocator(string_allocator)
    {
        m_command_map =
        {
            std::pair("PING", new ping_handler),
            std::pair("ECHO", new echo_handler),
            std::pair("GET", new get_handler<database>{ *this }),
            std::pair("SET", new set_handler<database>{ *this }),
        };
    }
}


std::string LambdaSnail::server::database::process_command(resp::data_view message)
{
    ZoneNamed(ProcessCommand, true);

    auto const request = message.materialize(resp::Array{});

    if (request.size() == 0 or request[0].type != LambdaSnail::resp::data_type::BulkString)
    {
        return {"-Unable to parse request\r\n"};
    }

    auto const _1 = request[0].materialize(resp::BulkString{});
    auto const cmd_it = m_command_map.find(_1);
    if (cmd_it != m_command_map.end())
    {
        auto *const command = cmd_it->second;
        if (command)
        {
            std::string s = command->execute(request);
            return s;
        }
    }

    return {"-Unable to find command\r\n"};
}

std::shared_ptr<LambdaSnail::server::entry_info> LambdaSnail::server::database::get_value(std::string const& key)
{
    auto lock = std::shared_lock{m_mutex};

    auto const it = m_store.find(key);
    if (it == m_store.end())
    {
        return nullptr;
    }

    if (it->second->has_ttl())
    {
        std::chrono::time_point<std::chrono::system_clock> const now = std::chrono::system_clock::now();
        if (it->second->ttl < now)
        {
            // Will be cleaned up in the background
            m_delete_keys.insert[key] = expiry_info
            {
                .version = it->second->version,
                .delete_reason = delete_reason::ttl_expiry
            };

            return nullptr;
        }
    }

    return it->second;
}

void LambdaSnail::server::database::set_value(std::string const& key, std::string_view value,
                                              std::chrono::time_point<std::chrono::system_clock> ttl)
{
    auto lock = std::shared_lock{m_mutex};

    auto const it = m_store.find(key);
    std::shared_ptr<entry_info> const value_wrapper = (it == m_store.end())
                                                          ? std::make_shared<entry_info>()
                                                          : it->second;

    value_wrapper->data = std::string(value);
    ++value_wrapper->version;
    value_wrapper->ttl = ttl;

    m_store[key] = value_wrapper;
}

void LambdaSnail::server::database::test_keys(time_point_t now, size_t max_num_tests)
{
    // For simplicity, we lock the entire database while performing maintenance
    auto lock = std::unique_lock{ m_mutex };

    // First check if we have deleted any keys or expired hem passively
    for (auto& [key, expiry] : m_delete_keys)
    {
        auto entry_it = m_store.find(key);
        if (entry_it == m_store.end()) [[unlikely]]
        {
            continue;
        }

        auto const entry = entry_it->second;

        // Even if the version differs, the entry may have expired, so we check for that
        // case as well, even if the delete reason is not due to an expired key
        if (entry->version != expiry.version and not entry->has_expired(now))
        {
            continue;
        }

        // The key could have been deleted multiple times for various reasons and from multiple
        // threads, so we need to check that it actually exists one more time
        auto const value_it = m_store.find(key);
        if (value_it != m_store.end())
        {
            m_store.unsafe_erase(key);
        }
    }

    m_delete_keys.clear();

    // Now we test a few keys at random to see if they are expired
    std::mt19937_64 random_engine(now);

    auto store_it = m_store.begin();
    size_t current_index = 0;
    for (size_t i = 0; i < max_num_tests; ++i)
    {
        auto const incr = random_engine();
        std::ranges::advance(store_it, static_cast<std::iter_difference_t<store_t::iterator>>(incr));
        if (store_it == m_store.end())
        {
            continue;
        }

        auto const value = store_it->second;
        if (value->has_expired(now))
        {
            store_it = m_store.unsafe_erase(store_it);
        }
    }
}

std::string LambdaSnail::server::ping_handler::execute(std::vector<resp::data_view> const &args) noexcept
{
    ZoneScoped;

    assert(args.size() == 1);
    return "+PONG\r\n";
}

std::string LambdaSnail::server::echo_handler::execute(std::vector<resp::data_view> const &args) noexcept
{
    ZoneScoped;

    assert(args.size() == 2);
    auto const str = args[1].materialize(resp::BulkString{});
    return "$" + std::to_string(str.size()) + "\r\n" + std::string(str.data(), str.size()) + "\r\n";
}

template<typename TDatabase>
std::string LambdaSnail::server::get_handler<TDatabase>::execute(std::vector<resp::data_view> const &args) noexcept
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
        auto option = args[3].materialize(resp::BulkString{}); // Assume EX or PX for now, also assume bulk string (can this be a simple string?)

        // Redis CLI sends a bulk string - need to refactor parsing part to handle various cases
        auto ttl_str = args[4].materialize(resp::BulkString{});

        //auto conversion = std::to_integer(ttl_str);
        int64_t ttl{};
        std::from_chars(ttl_str.data(), ttl_str.data() + ttl_str.length(), ttl);
        if (ttl == 0) [[unlikely]]
        {
            return "-Invalid option to SET command, EX and PX require a non-negative integer\r\n";
        }

        if (option == "EX")
        {
            m_database.set_value(key, value, std::chrono::system_clock::now() + std::chrono::seconds(ttl));
        } else
        {
            m_database.set_value(key, value, std::chrono::system_clock::now() + std::chrono::milliseconds(ttl));
        }

        return "+OK\r\n";
    }


    return "-Unable to SET\r\n";
}
