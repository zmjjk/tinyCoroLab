#include "coro/comp/condition_variable.hpp"

#include "coro/context.hpp"

namespace coro
{
bool condition_variable::cv_awaiter::register_lock() noexcept
{
    if (m_cond && m_cond())
    {
        return false;
    }

    register_cv();
    m_mtx.unlock();
    return true;
}

void condition_variable::cv_awaiter::register_cv() noexcept
{
    m_ctx.register_wait_task(m_register_state);
    m_register_state = false;
    m_next           = nullptr;

    if (m_cv.m_tail == nullptr)
    {
        m_cv.m_head = m_cv.m_tail = this;
    }
    else
    {
        m_cv.m_tail->m_next = this;
        m_cv.m_tail         = this;
    }
}

void condition_variable::cv_awaiter::wake_up() noexcept
{
    mutex_awaiter::register_lock();
}

void condition_variable::cv_awaiter::resume() noexcept
{
    if (m_cond && !m_cond())
    {
        register_cv();
        m_mtx.unlock();
        return;
    }
    mutex_awaiter::resume();
}

auto condition_variable::wait(mutex& mtx) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this);
}

auto condition_variable::wait(mutex& mtx, cond_type&& cond) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this, cond);
}

auto condition_variable::wait(mutex& mtx, cond_type& cond) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this, cond);
}

void condition_variable::notify_one() noexcept
{
    auto cur = m_head;
    if (cur != nullptr)
    {
        m_head = reinterpret_cast<cv_awaiter*>(m_head->m_next);
        if (m_head == nullptr)
        {
            m_tail = nullptr;
        }
        cur->wake_up();
    }
}

void condition_variable::notify_all() noexcept
{
    cv_awaiter* nxt{nullptr};
    auto        cur_head = m_head;
    m_head = m_tail = nullptr;
    while (cur_head != nullptr)
    {
        nxt = reinterpret_cast<cv_awaiter*>(cur_head->m_next);
        cur_head->wake_up();
        cur_head = nxt;
    }
}
} // namespace coro
