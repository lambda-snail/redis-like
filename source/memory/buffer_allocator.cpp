module;

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>

export module memory: memory.buffer_allocator;

namespace LambdaSnail::memory
{
    export
    template<size_t BufferSize>
    struct buffer_allocator
    {
        using value_type = std::array<char, BufferSize>*;

        buffer_allocator() noexcept;
        template <size_t U> buffer_allocator (buffer_allocator<U> const&) noexcept = delete;
        value_type* allocate (std::size_t n);
        void deallocate (value_type* p, std::size_t n);
    private:
        template<size_t buffer_size>
        struct allocation_information
        {
            std::array<char, buffer_size> buffer{};
            std::atomic<bool> isAllocated{false};
            size_t index{};
        };

        std::vector<allocation_information<BufferSize>> m_buffers {};
        std::shared_mutex m_mutex{};
    };
}

template<size_t BufferSize>
    LambdaSnail::memory::buffer_allocator<BufferSize>::buffer_allocator() noexcept
{
    m_buffers.resize(BufferSize);
    size_t index = 0;
    for(auto& allocation : m_buffers)
    {
        allocation.index = index++;
    }
}

template<size_t BufferSize>
typename LambdaSnail::memory::buffer_allocator<BufferSize>::value_type*
    LambdaSnail::memory::buffer_allocator<BufferSize>::allocate(std::size_t n)
{
    auto lock = std::unique_lock{ m_mutex };
    auto it = std::ranges::find_if(m_buffers.cbegin(), m_buffers.cend(), [](auto const& alloc){ return not alloc.isLocked; });
    if(it == m_buffers.cend())
    {
        return nullptr;
    }

    allocation_information<BufferSize>& free_buffer = *it;
    free_buffer.isAllocated = true;
    return &free_buffer.buffer;
}

template<size_t BufferSize>
void LambdaSnail::memory::buffer_allocator<BufferSize>::deallocate(value_type* p, std::size_t n)
{
    auto lock = std::shared_lock{m_mutex};
    auto* allocation = static_cast<allocation_information<BufferSize>>(p);

    assert(allocation->index >= 0 and allocation->index < BufferSize);
    assert(&m_buffers[allocation->index] == allocation);
    assert(allocation->isAllocated);

    allocation->isAllocated = false;
};
