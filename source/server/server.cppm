module;

#include <atomic>
#include <thread>
#include <future>
#include <shared_mutex>
#include <string>

#include "oneapi/tbb/concurrent_unordered_map.h"
//#include "oneapi/tbb/concurrent_set.h"

export module server;

import logging;
import memory;
import resp;

namespace LambdaSnail::server
{
    using time_point_t = std::chrono::time_point<std::chrono::system_clock>;

    struct entry_info
    {
        typedef uint32_t version_t;
        std::string data;
        version_t version{};
        time_point_t ttl{ time_point_t::min() };
        [[nodiscard]] bool has_ttl() const;
        [[nodiscard]] bool has_expired(time_point_t now) const;
    };

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

        // TODO: should probably return a variant or expected so we acn return an error as well
        [[nodiscard]] std::shared_ptr<entry_info> get_value(std::string const& key);

        void set_value(std::string const& key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl = {});

        /**
         * Implements the active expiry by testing some random keys in the database among the
         * possible keys with expiry.
         */
        void test_keys(time_point_t now, size_t max_num_tests = 10);

    private:
        memory::buffer_allocator<char>& m_string_allocator;

        store_t m_store{1000};

        enum class delete_reason : uint8_t
        {
            ttl_expiry = 0,
            user_deleted = 2 << 0
        };

        struct expiry_info
        {
            /**
             * The version of the value that should be deleted. If this differs from the actual value, then
             * we abort the operation.
             */
            entry_info::version_t version{};
            delete_reason delete_reason{};
        };

        /**
         * Keys with expiry are stored here as well. The actual expiry information
         */
        //tbb::concurrent_unordered_map<std::string, bool> m_ttl_keys;
        tbb::concurrent_unordered_map<std::string, expiry_info> m_delete_keys;

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

    /**
     * The timeout worker is the mechanism for active expiry of keys. At periodic intervals
     * it will test some random keys in the databases and if they are expired, remove them.
     */
    export class timeout_worker
    {
        // Periodically test a few keys from each database
        // Can we efficiently store keys that have timestamps?
    public:
        void do_work();

        void add_database(std::shared_ptr<database> database);
        //void remove_database(std::shared_ptr<database> database);

        ~timeout_worker();
    private:
        std::shared_ptr<database> m_database;
        std::thread m_worker_thread;
        std::shared_ptr<LambdaSnail::logging::logger> m_logger;
    };
}
