#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "coro/detail/types.hpp"
#include "coro/mutex_guard.hpp"

namespace coro
{

class context;

using detail::awaiter_ptr;

class mutex
{
public:
    struct mutex_awaiter
    {
        mutex_awaiter(context& ctx, mutex& mtx) noexcept : m_ctx(ctx), m_mtx(mtx), m_register_state(true) {}

        constexpr auto await_ready() noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        constexpr auto await_resume() noexcept -> void {}

        auto register_mutex() noexcept -> bool;

        auto resume() noexcept -> void;

        context&                m_ctx;
        mutex&                  m_mtx;
        mutex_awaiter*          m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
        bool                    m_register_state;
    };

    struct mutex_guard_awaiter : public mutex_awaiter
    {
        using guard_type = detail::lock_guard<mutex>;
        using mutex_awaiter::mutex_awaiter;

        auto await_resume() noexcept -> guard_type { return guard_type(m_mtx); }
    };

public:
    mutex() noexcept : m_state(nolocked), m_resume_list_head(nullptr) {}
    ~mutex() noexcept { assert(m_state.load(std::memory_order_acquire) == mutex::nolocked); }

    auto try_lock() noexcept -> bool;

    auto lock() noexcept -> mutex_awaiter;

    auto unlock() noexcept -> void;

    auto lock_guard() noexcept -> mutex_guard_awaiter;

private:
    friend mutex_awaiter;
    inline static awaiter_ptr nolocked          = reinterpret_cast<awaiter_ptr>(1);
    inline static awaiter_ptr locked_no_waiting = 0; // nullptr
    std::atomic<awaiter_ptr>  m_state;
    awaiter_ptr               m_resume_list_head;
};

}; // namespace coro
