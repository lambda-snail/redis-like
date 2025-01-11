module;

#include <iostream>
#include <vector>

export module memory: memory.buffer_allocator;

import :memory.buffer_pool;

namespace LambdaSnail::memory
{
    export
    template<typename T>
    struct buffer_allocator
    {
        using value_type = T;
        using pointer_type = T*;

        explicit buffer_allocator(buffer_pool& buffer_pool) noexcept;
        buffer_allocator (buffer_allocator<T> const&) noexcept = default;
        value_type* allocate (std::size_t n);
        void deallocate (value_type* p, std::size_t n);
    private:
        buffer_pool& m_buffer_pool;
    };
}

template<typename T>
LambdaSnail::memory::buffer_allocator<T>::buffer_allocator(buffer_pool& buffer_pool) noexcept : m_buffer_pool(buffer_pool) { }

template<typename T>
typename LambdaSnail::memory::buffer_allocator<T>::value_type*
    LambdaSnail::memory::buffer_allocator<T>::allocate(std::size_t n)
{
    auto const& [buffer, size] = m_buffer_pool.request_buffer(n);
    std::cout << "Allocated buffer " << buffer << " (" << size << ") bytes" << std::endl;

    return buffer;
}

template<typename T>
void LambdaSnail::memory::buffer_allocator<T>::deallocate(value_type* p, std::size_t n)
{
    m_buffer_pool.release_buffer(p);
    std::cout << "Released memory " << p << std::endl;
};
