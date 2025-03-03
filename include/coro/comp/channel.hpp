#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>

#include "coro/comp/condition_variable.hpp"
#include "coro/comp/mutex.hpp"
#include "coro/concepts/common.hpp"
#include "coro/log.hpp"
#include "coro/task.hpp"

namespace coro
{
template<concepts::conventional_type T, size_t capacity = 0>
class channel
{
    using data_type = std::optional<T>;

public:
    ~channel() noexcept
    {
        close();
        // FIXME: Maybe cause coredump
    }

    template<typename value_type>
        requires(std::is_constructible_v<T, value_type &&>)
    task<bool> send(value_type&& value) noexcept
    {
        if (closed())
        {
            co_return false;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_producer_cv.wait(m_mtx, [this]() -> bool { return !full() || m_closed; });
        if (m_closed)
        {
            co_return false;
        }

        m_array[m_tail] = std::forward<value_type>(value);
        m_tail++;
        m_num++;
        if (m_tail >= capacity)
        {
            m_tail = 0;
        }

        m_consumer_cv.notify_one();
        co_return true;
    }

    task<data_type> recv() noexcept
    {
        if (closed())
        {
            co_return std::nullopt;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_consumer_cv.wait(m_mtx, [this]() -> bool { return !empty() || m_closed; });
        if (m_closed)
        {
            co_return std::nullopt;
        }

        data_type p = std::make_optional<T>(std::move(m_array[m_head]));
        m_head++;
        m_num--;
        if (m_head >= capacity)
        {
            m_head = 0;
        }

        m_producer_cv.notify_one();
        co_return p;
    }

    void close() noexcept
    {
        std::atomic_ref<bool>(m_closed).store(true, std::memory_order_release);
        m_producer_cv.notify_all();
        m_consumer_cv.notify_all();
    }

private:
    inline bool empty() noexcept { return m_num == 0; }

    inline bool full() noexcept { return m_num == capacity; }

    inline bool closed() noexcept { return std::atomic_ref<bool>(m_closed).load(std::memory_order_acquire) == true; }

private:
    mutex                   m_mtx;
    condition_variable      m_producer_cv;
    condition_variable      m_consumer_cv;
    size_t                  m_head{0};
    size_t                  m_tail{0};
    size_t                  m_num{0};
    std::array<T, capacity> m_array;
    alignas(std::atomic_ref<bool>::required_alignment) bool m_closed{false};
};

template<concepts::conventional_type T, size_t capacity>
    requires(std::has_single_bit(capacity))
class channel<T, capacity>
{
    using data_type              = std::optional<T>;
    static constexpr size_t mask = capacity - 1;

public:
    ~channel() noexcept
    {
        close();
        // FIXME: Same problem as before
    }

    template<typename value_type>
        requires(std::is_constructible_v<T, value_type &&>)
    task<bool> send(value_type&& value) noexcept
    {
        if (closed())
        {
            co_return false;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_producer_cv.wait(m_mtx, [this]() -> bool { return !full() || m_closed; });

        if (m_closed)
        {
            co_return false;
        }

        m_array[tail()] = std::forward<value_type>(value);
        m_tail++;

        m_consumer_cv.notify_one();
        co_return true;
    }

    task<data_type> recv() noexcept
    {
        if (closed())
        {
            co_return std::nullopt;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_consumer_cv.wait(m_mtx, [this]() -> bool { return !empty() || m_closed; });
        if (m_closed)
        {
            co_return std::nullopt;
        }

        data_type p = std::make_optional<T>(std::move(m_array[head()]));
        m_head++;

        m_producer_cv.notify_one();
        co_return p;
    }

    void close() noexcept
    {
        std::atomic_ref<bool>(m_closed).store(true, std::memory_order_release);
        m_producer_cv.notify_all();
        m_consumer_cv.notify_all();
    }

private:
    inline size_t head() noexcept { return m_head & mask; }

    inline size_t tail() noexcept { return m_tail & mask; }

    inline bool empty() noexcept { return m_head == m_tail; }

    inline bool full() noexcept { return (m_tail - m_head) == capacity; }

    inline bool closed() noexcept { return std::atomic_ref<bool>(m_closed).load(std::memory_order_acquire) == true; }

private:
    mutex                   m_mtx;
    condition_variable      m_producer_cv;
    condition_variable      m_consumer_cv;
    size_t                  m_head{0};
    size_t                  m_tail{0};
    std::array<T, capacity> m_array;
    alignas(std::atomic_ref<bool>::required_alignment) bool m_closed{false};
};

template<concepts::conventional_type T>
class channel<T, 0>
{
    using data_type = std::optional<T>;

public:
    ~channel() noexcept
    {
        close();
        // FIXME: Same problem as before
    }

    template<typename value_type>
        requires(std::is_constructible_v<T, value_type &&>)
    task<bool> send(value_type&& value) noexcept
    {
        if (closed())
        {
            co_return false;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_producer_cv.wait(m_mtx, [this]() -> bool { return !(this->m_data.has_value()) || m_closed; });

        if (m_closed)
        {
            co_return false;
        }

        m_data = std::make_optional<T>(std::forward<value_type>(value));

        m_consumer_cv.notify_one();
        co_return true;
    }

    task<data_type> recv() noexcept
    {
        if (closed())
        {
            co_return std::nullopt;
        }

        auto lock = co_await m_mtx.lock_guard();
        co_await m_consumer_cv.wait(m_mtx, [this]() -> bool { return this->m_data.has_value() || m_closed; });

        if (m_closed)
        {
            co_return std::nullopt;
        }

        auto p = std::move(m_data);
        m_data = std::nullopt;
        m_producer_cv.notify_one();
        co_return p;
    }

    void close() noexcept
    {
        std::atomic_ref<bool>(m_closed).store(true, std::memory_order_release);
        m_producer_cv.notify_all();
        m_consumer_cv.notify_all();
    }

private:
    inline bool closed() noexcept { return std::atomic_ref<bool>(m_closed).load(std::memory_order_acquire) == true; }

private:
    mutex              m_mtx;
    condition_variable m_producer_cv;
    condition_variable m_consumer_cv;
    data_type          m_data{std::nullopt};
    alignas(std::atomic_ref<bool>::required_alignment) bool m_closed{false};
};

}; // namespace coro
