module;

#include <atomic>
#include <cassert>
#include <charconv>
#include <functional>
#include <future>
#include <iomanip>
#include <random>
#include <shared_mutex>
#include <string>

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <tracy/Tracy.hpp>

module server;

bool LambdaSnail::server::entry_info::has_ttl() const
{
    return ttl != std::chrono::time_point<std::chrono::system_clock>::min();
}

bool LambdaSnail::server::entry_info::has_expired(time_point_t now) const
{
    return ttl <= now;
}

bool LambdaSnail::server::entry_info::is_deleted() const
{
    return flags & static_cast<flags_t>(entry_flags::deleted);
}

void LambdaSnail::server::entry_info::set_deleted()
{
    flags |= static_cast<flags_t>(entry_flags::deleted);
}

std::shared_ptr<LambdaSnail::server::entry_info> LambdaSnail::server::database::get_value(std::string const& key)
{
    auto lock = std::shared_lock{ m_mutex };

    auto const it = m_store.find(key);
    if (it == m_store.end() or it->second->is_deleted())
    {
        return nullptr;
    }

    if (it->second->has_ttl())
    {
        std::chrono::time_point<std::chrono::system_clock> const now = std::chrono::system_clock::now();
        if (it->second->ttl < now)
        {
            // Will be cleaned up in the background by the maintenance thread
            m_delete_keys[key] = expiry_info
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
    value_wrapper->ttl = ttl;
    value_wrapper->flags = {};

    // Incrementing the version allows us to ignore the queue of entries to
    // be deleted, as the maintenance thread will see that the version is different
    // and abort the delete (unless exactly 2^32 sets are called before the next cleanup ...)
    ++value_wrapper->version;

    m_store[key] = value_wrapper;
}

void LambdaSnail::server::database::handle_deletes(time_point_t now, size_t max_num_tests)
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

        // Even if the version differs, the entry may have expired, so we check for that
        // case as well, even if the delete reason is not due to an expired key.
        // As an extra check we also abort the operation if the delete flag is not set.
        if ((entry_it->second->version != expiry.version
            or not entry_it->second->has_expired(now))
            or not entry_it->second->is_deleted())
        {
            continue;
        }

        // If we get here, we are confident the key can be deleted
        //m_store.unsafe_erase(entry_it);
        m_store.erase(entry_it);
    }

    m_delete_keys.clear();

    // A rare edge case perhaps, but no need to create random
    // generator if this holds true
    if (m_store.empty())
    {
        return;
    }

    // Now we test a few keys at random to see if they are expired
    std::mt19937_64 random_engine( static_cast<uint64_t>(now.time_since_epoch().count()) );
    std::uniform_int_distribution<size_t> distribution(0, m_store.size());

    for (size_t i = 0; i < max_num_tests; ++i)
    {
        auto store_it = m_store.begin();
        auto const incr = distribution(random_engine); //random_engine();
        std::advance(store_it, static_cast<std::iter_difference_t<store_t::iterator>>(incr));

        if (store_it == m_store.end())
        {
            break;
        }

        if (store_it->second->has_ttl() and store_it->second->has_expired(now))
        {
            m_store.erase(store_it);
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

    if (args.size() != 2)
    {
        // TODO: Need clean way to specify RESP string literals
        return "-Malformed ECHO command\r\n";
    }

    auto const str = args[1].materialize(resp::BulkString{});
    return "$" + std::to_string(str.size()) + "\r\n" + std::string(str.data(), str.size()) + "\r\n";
}

LambdaSnail::server::static_response_handler::static_response_handler(std::string_view message) noexcept : m_message(message) { }

std::string LambdaSnail::server::static_response_handler::execute(std::vector<resp::data_view> const &args) noexcept
{
    return std::string(m_message);
}

std::string LambdaSnail::server::get_handler::execute(std::vector<LambdaSnail::resp::data_view> const &args) noexcept
{
    ZoneScoped;

    if (args.size() == 2)
    {
        auto const key = std::string(args[1].materialize(LambdaSnail::resp::BulkString{}));

        auto value = m_database->get_value(std::move(key));
        if (value)
        {
            return value->data + "\r\n";
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
        auto value = args[2].value;
        m_database->set_value(key, value);
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
            m_database->set_value(key, value, std::chrono::system_clock::now() + std::chrono::seconds(ttl));
        } else
        {
            m_database->set_value(key, value, std::chrono::system_clock::now() + std::chrono::milliseconds(ttl));
        }

        return "+OK\r\n";
    }


    return "-Unable to SET\r\n";
}

std::string LambdaSnail::server::select_handler::execute(std::vector<resp::data_view> const &args) noexcept
{
    ZoneScoped;

    // TODO: Find nice way to encapsulate getting integers from bulk strings or parameters
    auto const database = args[1].materialize(resp::BulkString{});

    server::database_handle_t handle;
    std::from_chars(database.data(), database.data() + database.length(), handle);
    return m_dispatch.handle_set_database(handle);
}