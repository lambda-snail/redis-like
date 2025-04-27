module;

#include <shared_mutex>
#include <vector>
#include <list>
#include <optional>

#ifndef LS_NUM_BUCKETS
#define LS_NUM_BUCKETS 16
#endif

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

        static void move_impl(buffer_info& to, buffer_info& from) noexcept;
        void clear() noexcept;
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

        void release_buffer(buffer_info& buffer) noexcept;

    private:
        struct bucket
        {
            std::shared_mutex mutex{};
            size_t buffer_size{};
            size_t num_buffers{};
            std::vector<allocation_information> buffers{};
        };

        size_t m_lowest_size = 128;
        size_t m_num_buffers = 32;

        /**
         * Buckets with buffers holding commonly expected buffer sizes.
         */
        std::array<bucket, LS_NUM_BUCKETS> m_buckets;

        /**
         * List to hold allocations that do not fit in a bucket.
         */
        std::list<allocation_information> m_freelist{};
        std::shared_mutex m_freelist_mutex{};

        [[nodiscard]] std::optional<size_t> get_bucket(size_t n) const;
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