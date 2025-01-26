module;

#include <atomic>
#include <functional>
#include <future>
#include <shared_mutex>
#include <string>

#include "oneapi/tbb/concurrent_unordered_map.h"

export module server;
//export import :timeout_worker;

import memory;
import resp;

namespace LambdaSnail::server
{
    struct entry_info
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

    export struct ICommandHandler
    {
        [[nodiscard]] virtual std::string execute(std::vector<resp::data_view> const& args) noexcept = 0;
        virtual ~ICommandHandler() = default;
    };

    export class database
    {
    public:
        explicit database(memory::buffer_allocator<char>& string_allocator);
        [[nodiscard]] std::string process_command(resp::data_view message);

        [[nodiscard]] std::shared_ptr<entry_info> get_value(std::string const& key);

        void set_value(std::string const& key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl = {});

    private:
        memory::buffer_allocator<char>& m_string_allocator;

        store_t m_store{1000};

        // TODO: May need different structure for this when we can support multiple databases
        tbb::concurrent_unordered_map<std::string_view, ICommandHandler* const> m_command_map;

        /**
         * The key-value store is a concurrent queue, so is "safe" to access from multiple threads,
         * but we may periodically wish to perform maintenance work on the database, such as expire
         * key etc. In those cases the shared mutex allows us to lock the entire map for a short
         * duration.
         */
        mutable std::shared_mutex m_mutex{};
    };
}