module;

#include <array>
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <vector>

export module memory: memory.buffer_pool;

namespace LambdaSnail::memory
{
    export struct buffer_info
    {
        char* buffer{};
        size_t size{};
    };

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
    auto it = std::ranges::find_if(m_buffers.begin(), m_buffers.end(), [](auto const& alloc){ return not alloc.isAllocated; });
    if(it == m_buffers.end())
    {
        return { .buffer = nullptr, .size = 0 };;
    }

    allocation_information<1024>& free_buffer = *it;
    free_buffer.isAllocated = true;

    //std::cout << "Allocate: " << &free_buffer.buffer << std::endl;
    return { .buffer = free_buffer.buffer.data(), .size = free_buffer.buffer.size() };
}

void LambdaSnail::memory::buffer_pool::release_buffer(char *buffer) noexcept
{
    auto lock = std::shared_lock{m_mutex};
    auto* allocation = reinterpret_cast<allocation_information<1024>*>(buffer);

    assert(allocation->index >= 0 and allocation->index < BufferSize);
    assert(&m_buffers[allocation->index] == allocation);
    assert(allocation->isAllocated);

    allocation->isAllocated = false;
    //std::cout << "Deallocate: " << p << std::endl;
}
