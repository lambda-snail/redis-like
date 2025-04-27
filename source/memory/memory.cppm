module;

#include <shared_mutex>
#include <vector>

export module memory;

namespace LambdaSnail::memory
{
    export class buffer_pool;

    struct allocation_information
    {
        std::vector<char> buffer{};
        size_t index{};
        bool isAllocated{false};
    };

    // TODO: RAII enable: make it call release?
    // (but then we need to prevent copying? Maybe better to use a shared_ptr with custom deallocator?)
    export struct buffer_info
    {
        char* buffer{};
        size_t size{};

        buffer_info() = default;
        buffer_info(char* buffer_, size_t size_, buffer_pool* pool_, allocation_information* info_);
        buffer_info(buffer_info&& info) noexcept;
        buffer_info& operator=(buffer_info&&) noexcept;

        ~buffer_info();
        buffer_info(const buffer_info& info) = delete;
        buffer_info& operator=(buffer_info const&) = delete;

    private:
        friend buffer_pool;
        buffer_pool* m_pool{};

        allocation_information* m_allocation_info{};

        void move_impl(buffer_info& to, buffer_info& from) noexcept;
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

        [[nodiscard]] buffer_info request_buffer(size_t size) noexcept;

        void release_buffer(buffer_info const& buffer) noexcept;

    private:
        struct bucket
        {
            std::shared_mutex mutex{};
            size_t buffer_size{};
            size_t num_buffers{};
            std::vector<allocation_information> buffers{};
        };

        /**
         * Indices 0-15
         *
         */
        std::array<bucket, 16> m_buckets;

        // std::vector<allocation_information<1024>> m_buffers{};
        // std::shared_mutex m_mutex{};
        [[nodiscard]] size_t get_bucket(size_t n) const;

        size_t m_lowest_size = 128;
        size_t m_num_buffers = 64;
    };

    export template<typename T>
    struct buffer_allocator
    {
        using value_type   = T;
        using pointer_type = T*;

        explicit buffer_allocator(buffer_pool& buffer_pool) noexcept;

        buffer_allocator(buffer_allocator<T> const&) noexcept = default;

        value_type* allocate(std::size_t n);

        void deallocate(value_type* p, std::size_t n);

    private:
        buffer_pool& m_buffer_pool;
    };
} // namespace LambdaSnail::memory