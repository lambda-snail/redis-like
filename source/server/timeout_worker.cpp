module;

#include <chrono>
#include <queue>

module server;

//import :database;

namespace LambdaSnail::server
{
    struct timeout_info
    {
        std::string key;
        entry_info::version_t version;
        std::chrono::time_point<std::chrono::system_clock> ttl_time_stamp;
        database& database;
        [[nodiscard]] bool constexpr operator<(timeout_info const& other) const;
    };

    bool constexpr timeout_info::operator<(timeout_info const& other) const
    {
        return ttl_time_stamp < other.ttl_time_stamp;
    }

    class timeout_worker
    {
        // Priority heap of time stamps
        // The data needs time stamp, database id, key, and key version
        // Trigger maintenance function periodically:
        //  - Peek the top of the heap, if the time has not yet passed, go back to sleep
        //  - Else, pop one by one until all expired entries are processed
        //      - Lock database
        //      - Find key
        //      - Must check key version so that it hasn't been updated
    private:
        std::priority_queue<timeout_info> m_heap {};

    };
}
