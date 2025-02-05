module;

#include <atomic>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <utility>

export module server;

import logging;
import memory;
import resp;

namespace LambdaSnail::server
{
    export typedef std::chrono::time_point<std::chrono::system_clock> time_point_t;

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

    struct ICommandHandler
    {
        [[nodiscard]] virtual std::string execute(std::vector<resp::data_view> const& args) noexcept = 0;
        virtual ~ICommandHandler() = default;
    };

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

    struct static_response_handler final : public ICommandHandler
    {
        explicit static_response_handler(std::string_view) noexcept;
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;
        ~static_response_handler() override = default;
    private:
        std::string m_message;
    };

    //template<typename TDatabase>
    struct get_handler final : public ICommandHandler
    {
        explicit get_handler(std::shared_ptr<class database> database) noexcept : m_database(std::move(database)) { }
        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;
        ~get_handler() override = default;

    private:
        std::shared_ptr<database> m_database;
    };

    //template<typename TDatabase>
    struct set_handler final : public ICommandHandler
    {
        explicit set_handler(std::shared_ptr<database> database) : m_database(database)
        {
        }

        [[nodiscard]] std::string execute(std::vector<resp::data_view> const &args) noexcept override;

        ~set_handler() override = default;

    private:
        std::shared_ptr<database> m_database;
    };



    export class database
    {
    public:
        explicit database();
        //[[nodiscard]] std::string process_command(resp::data_view message);

        // TODO: should probably return a variant or expected so we can return an error as well
        [[nodiscard]] std::shared_ptr<entry_info> get_value(std::string const& key);

        void set_value(std::string const& key, std::string_view value, std::chrono::time_point<std::chrono::system_clock> ttl = {});

        /**
         * Implements the active expiry by testing some random keys in the database among the
         * possible keys with expiry.
         */
        void handle_deletes(time_point_t now, size_t max_num_tests = 10);

    private:
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

    /**
     * A server is a collection of databases and the member functions used to manage these.
     */
    export class server
    {
    public:
        typedef size_t database_handle_t;
        typedef size_t database_size_t;
        typedef std::vector<std::shared_ptr<database>>::const_iterator database_iterator_t;

        database_handle_t create_database();
        [[nodiscard]] std::shared_ptr<database> get_database(database_handle_t database_no) const;
        //[[nodiscard]] database_size_t get_database_size(database_handle_t database_no) const;

        [[nodiscard]] database_iterator_t begin() const;
        [[nodiscard]] database_iterator_t end() const;

    private:
        std::vector<std::shared_ptr<database>> m_databases{};
    };

    export class command_dispatch
    {
    public:
        explicit command_dispatch(server& server);
        [[nodiscard]] std::string process_command(resp::data_view message);

        void set_database(server::database_handle_t index);
    private:
        [[nodiscard]] std::shared_ptr<ICommandHandler> get_command(std::string_view command_name) const;

        static std::unordered_map<std::string_view, std::function<ICommandHandler*()>> s_command_map;
        server& m_server;

        server::database_handle_t m_current_db{};
    };

    /**
     * The timeout worker is the mechanism for active expiry of keys. At periodic intervals
     * it will test some random keys in the databases and if they are expired, remove them.
     */
    export class timeout_worker
    {
    public:
        explicit timeout_worker(server& server, std::shared_ptr<LambdaSnail::logging::logger> m_logger);

        /**
         * Periodically cleans up pending deletes and tests a few random keys from each database
         * for expiry.
         */
        void do_work() const;
        [[nodiscard]] std::future<void> do_work_async() const;
    private:
        LambdaSnail::server::server& m_server;
        std::shared_ptr<LambdaSnail::logging::logger> m_logger{};
    };
}
