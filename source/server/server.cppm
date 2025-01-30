module;

#include <atomic>
#include <expected>
#include <unordered_map>
#include <thread>
#include <future>
#include <shared_mutex>
#include <string>

export module server;

import logging;
import memory;
import resp;

namespace LambdaSnail::server
{
    using time_point_t = std::chrono::time_point<std::chrono::system_clock>;

    struct entry_info
    {
        enum class entry_flags
        {
            no_state = 0,
            deleted = 1 << 0
        };

        typedef uint32_t version_t;
        typedef uint32_t flags_t;

        std::string data;
        version_t version{};
        flags_t flags{};
        time_point_t ttl{ time_point_t::min() };
        [[nodiscard]] bool has_ttl() const;
        [[nodiscard]] bool has_expired(time_point_t now) const;
        [[nodiscard]] bool is_deleted() const;
        void set_deleted();
    };

    using store_t = std::unordered_map<std::string, std::shared_ptr<entry_info>>;

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

        // TODO: should probably return a variant or expected so we can return an error as well
        [[nodiscard]] std::shared_ptr<entry_info> get_value(std::string const& key);

        void set_value(std::string const& key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl = {});

        /**
         * Implements the active expiry by testing some random keys in the database among the
         * possible keys with expiry.
         */
        void handle_deletes(time_point_t now, size_t max_num_tests = 10);

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
         * Instead of deleting key/values from the database, key/values to be deleted are added
         * to this list and cleaned up from a background thread. This avoids the need for synchronizing
         * updates in two concurrent data structures during regular operations (GET, SET).
         * TODO: Use queue instead?
         */
        std::unordered_map<std::string, expiry_info> m_delete_keys;

        // TODO: May need different structure for this when we can support multiple databases
        std::unordered_map<std::string_view, ICommandHandler* const> m_command_map;

        /**
         * The key-value store is a concurrent queue, so is "safe" to access from multiple threads,
         * but we may periodically wish to perform maintenance work on the database, such as expire
         * key etc. In those cases the shared mutex allows us to lock the entire map for a short
         * duration.
         */
        mutable std::shared_mutex m_mutex{};
    };

    class server
    {
    public:
        typedef size_t database_handle_t;
        typedef size_t database_size_t;
        typedef std::vector<std::shared_ptr<database>>::iterator database_iterator_t;

        database_handle_t create_database();
        [[nodiscard]] database_iterator_t get_database(database_handle_t database_no) const;
        [[nodiscard]] database_size_t get_database_size() const;

        [[nodiscard]] database_iterator_t begin() const;
        [[nodiscard]] database_iterator_t end() const;

    private:
        std::vector<std::shared_ptr<database>> m_databases{};
    };

    /**
     * The timeout worker is the mechanism for active expiry of keys. At periodic intervals
     * it will test some random keys in the databases and if they are expired, remove them.
     */
    export class timeout_worker
    {
    public:
        explicit timeout_worker(std::shared_ptr<LambdaSnail::logging::logger> m_logger);

        /**
         * Periodically cleans up pending deletes and tests a few random keys from each database
         * for expiry.
         */
        void do_work() const;
        [[nodiscard]] std::future<void> do_work_async() const;

        void add_database(std::shared_ptr<database> database);
    private:
        std::shared_ptr<database> m_database; // TODO: Replace with reference to server that can fetch database list (copy to new thread)
        std::shared_ptr<LambdaSnail::logging::logger> m_logger{};
    };
}
