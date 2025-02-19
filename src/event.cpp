#include "coro/event.hpp"
#include "coro/context.hpp"

namespace coro
{

bool event::awaiter::await_suspend(std::coroutine_handle<> handle) noexcept
{
    m_await_coro = handle;
    return m_ev.register_awaiter(this);
}

event::awaiter event::wait() noexcept
{
    return event::awaiter(local_context(), *this);
}

void event::set() noexcept
{
    auto flag = m_state.exchange(this, std::memory_order_acq_rel);
    if (flag != this)
    {
        auto waiter = static_cast<awaiter*>(flag);
        resume_all_awaiter(waiter);
    }
}

void event::resume_all_awaiter(awaiter_ptr waiter) noexcept
{
    while (waiter != nullptr)
    {
        auto cur = static_cast<awaiter*>(waiter);
        cur->m_ctx.submit_task(cur->m_await_coro);
        cur->m_ctx.unregister_wait_task();
        waiter = cur->m_next;
    }
}

bool event::register_awaiter(awaiter* waiter) noexcept
{
    const auto  set_state = this;
    awaiter_ptr old_value = nullptr;

    do
    {
        old_value = m_state.load(std::memory_order_acquire);
        if (old_value == this)
        {
            waiter->m_next = nullptr;
            return false;
        }
        waiter->m_next = static_cast<awaiter*>(old_value);
    } while (!m_state.compare_exchange_weak(old_value, waiter, std::memory_order_acquire));

    waiter->m_ctx.register_wait_task();
    return true;
}

}; // namespace coro
