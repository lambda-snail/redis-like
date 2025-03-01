module;

#include <array>
#include <cassert>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <vector>

module memory;

LambdaSnail::memory::buffer_pool::buffer_pool()
{
    m_buffers.resize(1024);
    size_t index = 0;
    for (auto& allocation: m_buffers)
    {
        allocation.index = index++;
    }
}

LambdaSnail::memory::buffer_info LambdaSnail::memory::buffer_pool::request_buffer(size_t size) noexcept
{
    auto lock     = std::unique_lock{m_mutex};
    auto const it = std::ranges::find_if(m_buffers.begin(), m_buffers.end(),
                                         [](auto const& alloc) { return not alloc.isAllocated; });
    if (it == m_buffers.end())
    {
        return {.buffer = nullptr, .size = 0};
        ;
    }

    auto& [buffer, isAllocated, index] = *it;

    assert(not isAllocated);
    isAllocated = true;

    // std::cout << "Allocate: " << &free_buffer.buffer << std::endl;
    return {.buffer = buffer.data(), .size = buffer.size()};
}

void LambdaSnail::memory::buffer_pool::release_buffer(char* buffer) noexcept
{
    auto lock        = std::shared_lock{m_mutex};
    auto* allocation = reinterpret_cast<allocation_information<1024>*>(buffer);

    assert(allocation->index < 1024);
    assert(&m_buffers[allocation->index] == allocation);
    assert(allocation->isAllocated);

    allocation->isAllocated = false;
    // std::cout << "Deallocate: " << p << std::endl;
}
