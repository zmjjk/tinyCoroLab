#pragma once
#include <atomic>
#include <coroutine>

#include "coro/concepts/awaitable.hpp"
#include "coro/detail/types.hpp"

namespace coro
{

using detail::awaiter_ptr;

class context;

class event
{
public:
    struct awaiter
    {
        awaiter(context& ctx, event& e) noexcept : m_ctx(ctx), m_ev(e) {}
        inline awaiter* next() noexcept { return m_next; }

        inline bool await_ready() const noexcept { return m_ev.is_set(); }

        bool await_suspend(std::coroutine_handle<> handle) noexcept;

        constexpr void await_resume() noexcept {}

        context&                m_ctx;
        event&                  m_ev;
        awaiter*                m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
    };

    event() noexcept  = default;
    ~event() noexcept = default;

    event(const event&)            = delete;
    event(event&&)                 = delete;
    event& operator=(const event&) = delete;
    event& operator=(event&&)      = delete;

    [[CORO_AWAIT_HINT]] awaiter wait() noexcept;

    inline bool is_set() const noexcept { return m_state.load(std::memory_order_acquire) == this; }

    void set() noexcept;

    void resume_all_awaiter(awaiter_ptr waiter) noexcept;

    bool register_awaiter(awaiter* waiter) noexcept;

private:
    std::atomic<awaiter_ptr> m_state{nullptr};
};

}; // namespace coro
