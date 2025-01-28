module;

#include <chrono>
#include <future>
#include <stdexcept>

module server;

namespace LambdaSnail::server
{
    timeout_worker::timeout_worker(std::shared_ptr<LambdaSnail::logging::logger> logger) : m_logger(logger)
    {
        if (not m_logger)
        {
            throw std::invalid_argument("The provided logger is nullptr");
        }
    }

    void timeout_worker::do_work() const
    {
        m_logger->get_system_logger()->info("Database maintenance thread started");

        time_point_t now = std::chrono::system_clock::now();
        if (m_database)
        {
            m_logger->get_system_logger()->trace("Performing maintenance on database 0");
            m_database->handle_deletes(now);
        }
    }

    std::future<void> timeout_worker::do_work_async() const
    {
        return std::async(std::launch::async, [&](){ do_work(); });
    }

    // TODO: Remove function and fetch list of dbs in do_work
    void timeout_worker::add_database(std::shared_ptr<database> database)
    {
        m_database = std::move(database);
    }
}
