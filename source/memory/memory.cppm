module;

#include <shared_mutex>
#include <vector>

export module memory;

namespace LambdaSnail::memory
{
    export struct buffer_info final
    {
        buffer_info(char* buf, size_t len, class buffer_pool& pool) : buffer(buf), size(len), m_buffer_pool(pool) {}

        char* buffer{};
        size_t size{};

        buffer_info(const buffer_info& info) = delete;
        buffer_info& operator =(buffer_info const&) = delete;

        ~buffer_info();
    private:
        buffer_pool& m_buffer_pool;
    };

    /**
     * Pool of buffers for small to medium sizes
     * Keep track of how many available in each bucket?
     *
     */
    export class buffer_pool
    {
    public:
        explicit buffer_pool();

        [[nodiscard]] buffer_info request_buffer() noexcept;

        void release_buffer(char* buffer) noexcept;

    private:
        template<size_t buffer_size>
        struct allocation_information
        {
            std::array<char, buffer_size> buffer{};
            bool isAllocated{false};
            size_t index{};
        };

        std::vector<allocation_information<1024>> m_buffers{};
        std::shared_mutex m_mutex{};
    };
} // namespace LambdaSnail::memory