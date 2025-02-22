#include "coro/mutex.hpp"
#include "coro/context.hpp"

namespace coro
{
auto mutex::mutex_awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_await_coro = handle;
    return register_mutex();
}

auto mutex::mutex_awaiter::register_mutex() noexcept -> bool
{
    while (true)
    {
        auto state = m_mtx.m_state.load(std::memory_order_acquire);
        m_next     = nullptr;
        if (state == mutex::nolocked)
        {
            if (m_mtx.m_state.compare_exchange_weak(
                    state, mutex::locked_no_waiting, std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                return false;
            }
        }
        else
        {
            m_next = reinterpret_cast<mutex_awaiter*>(state);
            if (m_mtx.m_state.compare_exchange_weak(
                    state, reinterpret_cast<awaiter_ptr>(this), std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                m_ctx.register_wait_task(m_register_state);
                m_register_state = false;
                return true;
            }
        }
    }
}

auto mutex::mutex_awaiter::resume() noexcept -> void
{
    m_ctx.submit_task(m_await_coro);
    m_ctx.unregister_wait_task();
    m_register_state = true;
}

auto mutex::try_lock() noexcept -> bool
{
    auto target = nolocked;
    return m_state.compare_exchange_strong(target, locked_no_waiting, std::memory_order_acq_rel, memory_order_relaxed);
}

auto mutex::lock() noexcept -> mutex_awaiter
{
    return mutex_awaiter(local_context(), *this);
}

auto mutex::unlock() noexcept -> void
{
    assert(m_state.load(std::memory_order_acquire) != nolocked);
    mutex_awaiter* awaiter{nullptr};
    while (true)
    {
        auto target = locked_no_waiting;
        if (m_state.compare_exchange_weak(target, nolocked, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            return;
        }
        auto state = m_state.load();
        awaiter    = reinterpret_cast<mutex_awaiter*>(state);
        auto nxt   = reinterpret_cast<awaiter_ptr>(awaiter->m_next);
        if (m_state.compare_exchange_weak(state, nxt, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            break;
        }
    }
    if (awaiter != nullptr)
    {
        awaiter->resume();
    }
}

auto mutex::lock_guard() noexcept -> mutex_guard_awaiter
{
    return mutex_guard_awaiter(local_context(), *this);
}
}; // namespace coro