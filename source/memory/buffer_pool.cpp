module;

#include <array>
#include <cassert>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <vector>

module memory;

LambdaSnail::memory::buffer_info& LambdaSnail::memory::buffer_info::operator=(buffer_info&& other) noexcept
{
    move_impl(*this, other);
    return *this;
}

LambdaSnail::memory::buffer_info::~buffer_info()
{
    assert(m_pool);
    m_pool->release_buffer(*this);
}

LambdaSnail::memory::buffer_info::buffer_info(char* buffer_, size_t size_, buffer_pool* pool_, allocation_information* info_)
    : buffer(buffer_), size(size_), m_pool(pool_), m_allocation_info(info_) { }

LambdaSnail::memory::buffer_info::buffer_info(buffer_info&& other) noexcept
{
    move_impl(*this, other);
}

void LambdaSnail::memory::buffer_info::move_impl(buffer_info& to, buffer_info& from) noexcept
{
    to.buffer = from.buffer;
    from.buffer = nullptr;

    to.size = from.size;
    from.size = 0;

    to.m_pool = from.m_pool;
    from.m_pool = nullptr;

    to.m_allocation_info = from.m_allocation_info;
    from.m_allocation_info = nullptr;
}

LambdaSnail::memory::buffer_pool::buffer_pool()
{
    // 16 indices
    // indices 0-14 have fixed sizes
    // last one (15) is catch-all for everything else, only allocated when needed

    for (int i = 0; i < m_buckets.size() - 1; ++i)
    {
        m_buckets[i].buffer_size = m_lowest_size << i;
        m_buckets[i].num_buffers = m_num_buffers;

        m_buckets[i].buffers.resize(m_num_buffers);
        size_t index = 0;
        for (auto& allocation: m_buckets[i].buffers)
        {
            allocation.index = index++;
            allocation.buffer.resize(m_buckets[i].buffer_size);
        }
    }

    m_buckets[m_buckets.size() - 1].buffer_size = 0;
    m_buckets[m_buckets.size() - 1].num_buffers = 0;
}

size_t LambdaSnail::memory::buffer_pool::get_bucket(size_t n) const
{
    n /= m_lowest_size;
    for (size_t i = 0; i < m_buckets.size(); ++i)
    {
        if (n < (1 << i))
        {
            return i;
        }
    }

    return m_buckets.size() - 1;
}

LambdaSnail::memory::buffer_info LambdaSnail::memory::buffer_pool::request_buffer(size_t size) noexcept
{
    size_t bucket = get_bucket(size);
    auto lock     = std::unique_lock{m_buckets[bucket].mutex};

    // TODO: First search through bucket to see if possible allocation exists
    if (bucket == m_buckets.size() - 1)
    {
        auto& info = m_buckets[bucket].buffers.emplace_back(std::vector<char>{}, size, true);
        info.buffer.resize(size);

        return { info.buffer.data(), info.buffer.size(), this , &info };
    }

    auto const it = std::ranges::find_if(m_buckets[bucket].buffers.begin(), m_buckets[bucket].buffers.end(),
                                         [](auto const& alloc) { return not alloc.isAllocated; });

    if (it == m_buckets[bucket].buffers.end())
    {
        return {};
    }

    assert(not it->isAllocated);
    it->isAllocated = true;

    return { it->buffer.data(), it->buffer.size(), this, std::addressof(*it) };
}

void LambdaSnail::memory::buffer_pool::release_buffer(buffer_info const& buffer) noexcept
{
    size_t bucket = get_bucket(buffer.size);
    auto lock     = std::shared_lock{m_buckets[bucket].mutex};

    assert(buffer.m_pool == this);
    assert(buffer.m_allocation_info->isAllocated);

    buffer.m_allocation_info->isAllocated = false;
}