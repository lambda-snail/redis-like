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
    // The user may have called release manually so we need to guard against this case
    if (m_pool)
    {
        m_pool->release_buffer(*this);
    }
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
    to.size = from.size;
    to.m_pool = from.m_pool;
    to.m_allocation_info = from.m_allocation_info;

    from.clear();
}

void LambdaSnail::memory::buffer_info::clear() noexcept
{
    buffer = nullptr;
    size = 0;
    m_allocation_info = nullptr;
    m_pool = nullptr;
}

LambdaSnail::memory::buffer_pool::buffer_pool()
{
    for (int i = 0; i < m_buckets.size(); ++i)
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
}

std::optional<size_t> LambdaSnail::memory::buffer_pool::get_bucket(size_t n) const
{
    n /= m_lowest_size;
    for (size_t i = 0; i < m_buckets.size(); ++i)
    {
        if (n < (1 << i))
        {
            return i;
        }
    }

    return {};
}

LambdaSnail::memory::buffer_info LambdaSnail::memory::buffer_pool::request_buffer(size_t size) noexcept
{
    auto maybe_bucket = get_bucket(size);

    if (not maybe_bucket.has_value())
    {
        std::unique_lock lock{m_freelist_mutex};
        auto& info = m_freelist.emplace_back(std::vector<char>{}, size, true);
        info.buffer.resize(size);

        return { info.buffer.data(), info.buffer.size(), this , &info };
    }

    auto bucket = maybe_bucket.value();
    std::unique_lock lock{m_buckets[bucket].mutex};

    // TODO: If we cannot find available allocation, we need to keep checking larger buckets until we find one
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

void LambdaSnail::memory::buffer_pool::release_buffer(buffer_info& buffer) noexcept
{
    assert(buffer.m_pool == this);
    assert(buffer.m_allocation_info->isAllocated);

    auto maybe_bucket = get_bucket(buffer.size);

    if (maybe_bucket.has_value())
    {
        size_t bucket = maybe_bucket.value();
        std::shared_lock lock{m_buckets[bucket].mutex};
        buffer.m_allocation_info->isAllocated = false;
    }
    else
    {
        std::shared_lock lock{m_freelist_mutex};
        auto num_removed = m_freelist.remove_if([&buffer](allocation_information const& info) { return buffer.m_allocation_info == std::addressof(info); });
        assert(num_removed == 1);
    }

    buffer.clear();
}