#pragma once
#include <atomic>
#include <coroutine>

#include "coro/attribute.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/context.hpp"
#include "coro/detail/container.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
class context;

namespace detail
{
class event_base
{
public:
    struct awaiter_base
    {
        awaiter_base(context& ctx, event_base& e) noexcept : m_ctx(ctx), m_ev(e) {}
        inline awaiter_base* next() noexcept { return m_next; }

        inline bool await_ready() const noexcept { return m_ev.is_set(); }

        bool await_suspend(std::coroutine_handle<> handle) noexcept;

        constexpr void await_resume() noexcept {}

        context&                m_ctx;
        event_base&             m_ev;
        awaiter_base*           m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
    };

    event_base() noexcept  = default;
    ~event_base() noexcept = default;

    event_base(const event_base&)            = delete;
    event_base(event_base&&)                 = delete;
    event_base& operator=(const event_base&) = delete;
    event_base& operator=(event_base&&)      = delete;

    inline bool is_set() const noexcept { return m_state.load(std::memory_order_acquire) == this; }

    void set_state() noexcept;

    void resume_all_awaiter(awaiter_ptr waiter) noexcept;

    bool register_awaiter(awaiter_base* waiter) noexcept;

private:
    std::atomic<awaiter_ptr> m_state{nullptr};
};
}; // namespace detail

template<typename return_type = void>
class event : public detail::event_base, public detail::container<return_type>
{
public:
    struct [[CORO_AWAIT_HINT]] awaiter : public detail::event_base::awaiter_base
    {
        using awaiter_base::awaiter_base;
        auto await_resume() noexcept -> decltype(auto) { return static_cast<event&>(m_ev).result(); }
    };

    [[CORO_AWAIT_HINT]] awaiter wait() noexcept { return awaiter(local_context(), *this); }

    template<typename value_type>
    void set(value_type&& value) noexcept
    {
        this->return_value(std::forward<value_type>(value));
        set_state();
    }
};

template<>
class event<void> : public detail::event_base
{
public:
    struct [[CORO_AWAIT_HINT]] awaiter : public detail::event_base::awaiter_base
    {
        using awaiter_base::awaiter_base;
        constexpr void await_resume() noexcept {};
    };

    [[CORO_AWAIT_HINT]] awaiter wait() noexcept { return awaiter(local_context(), *this); }

    void set() noexcept { set_state(); }
};

}; // namespace coro
