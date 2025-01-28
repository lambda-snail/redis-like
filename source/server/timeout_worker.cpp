module;

#include <chrono>

module server;

//import logging;

namespace LambdaSnail::server
{
    void timeout_worker::do_work()
    {
        time_point_t now = std::chrono::system_clock::now();
        if (m_database)
        {
            m_database->handle_deletes(now);
        }
    }

    void timeout_worker::add_database(std::shared_ptr<database> database)
    {
        //m_databases.push_back(database);

    }

    timeout_worker::~timeout_worker()
    {
        m_worker_thread.join();
    }
}
