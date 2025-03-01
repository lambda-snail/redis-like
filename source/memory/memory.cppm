module;

#include <shared_mutex>
#include <vector>

export module memory;

namespace LambdaSnail::memory
{
    // TODO: RAII enable: make it call release?
    // (but then we need to prevent copying? Maybe better to use a shared_ptr with custom deallocator?)
    export struct buffer_info
    {
        char* buffer{};
        size_t size{};
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

        void release_buffer(char *buffer) noexcept;

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

    export template<typename T> struct buffer_allocator
    {
        using value_type = T;
        using pointer_type = T *;

        explicit buffer_allocator(buffer_pool &buffer_pool) noexcept;

        buffer_allocator(buffer_allocator<T> const &) noexcept = default;

        value_type *allocate(std::size_t n);

        void deallocate(value_type *p, std::size_t n);

    private:
        buffer_pool &m_buffer_pool;
    };
}
