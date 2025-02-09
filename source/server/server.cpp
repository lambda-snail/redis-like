module;

#include <cassert>
#include <memory>

module server;

namespace LambdaSnail::server
{
    server::database_handle_t server::create_database()
    {
        m_databases.emplace_back(std::make_shared<database>());
        return m_databases.size() - 1;
    }

    std::shared_ptr<database> server::get_database(database_handle_t database_no) const
    {
        assert(database_no < m_databases.size());
        return m_databases[database_no];
    }

    bool server::is_valid_handle(database_handle_t database_no) const
    {
        return database_no < m_databases.size();
    }

    // server::database_size_t server::get_database_size(database_handle_t database_no) const
    // {
    //     assert(database_no < m_databases.size());
    //     return m_databases[database_no]->size();
    // }

    server::database_iterator_t server::begin() const
    {
        return m_databases.cbegin();
    }

    server::database_iterator_t server::end() const
    {
        return m_databases.cend();
    }
};