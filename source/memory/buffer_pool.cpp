module;

#include <array>
#include <cassert>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <vector>

export module memory: memory.buffer_pool;

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
        void release_buffer(char* buffer) noexcept;

    private:
        template<size_t buffer_size>
        struct allocation_information
        {
            std::array<char, buffer_size> buffer{};
            bool isAllocated{false};
            size_t index{};
        };

        std::vector<allocation_information<1024>> m_buffers {};
        std::shared_mutex m_mutex{};
    };
}

LambdaSnail::memory::buffer_pool::buffer_pool()
{
    m_buffers.resize(1024);
    size_t index = 0;
    for(auto& allocation : m_buffers)
    {
        allocation.index = index++;
    }
}

LambdaSnail::memory::buffer_info
LambdaSnail::memory::buffer_pool::request_buffer(size_t size) noexcept
{
    auto lock = std::unique_lock{ m_mutex };
    auto const it = std::ranges::find_if(m_buffers.begin(), m_buffers.end(),
        [](auto const& alloc){ return not alloc.isAllocated; });
    if(it == m_buffers.end())
    {
        return { .buffer = nullptr, .size = 0 };;
    }

    auto& [buffer, isAllocated, index] = *it;

    assert(not isAllocated);
    isAllocated = true;

    //std::cout << "Allocate: " << &free_buffer.buffer << std::endl;
    return { .buffer = buffer.data(), .size = buffer.size() };
}

void LambdaSnail::memory::buffer_pool::release_buffer(char *buffer) noexcept
{
    auto lock = std::shared_lock{m_mutex};
    auto* allocation = reinterpret_cast<allocation_information<1024>*>(buffer);

    assert(allocation->index < 1024);
    assert(&m_buffers[allocation->index] == allocation);
    assert(allocation->isAllocated);

    allocation->isAllocated = false;
    //std::cout << "Deallocate: " << p << std::endl;
}
