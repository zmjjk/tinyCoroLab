#pragma once

#include <functional>

#include "coro/comp/mutex.hpp"

#include "coro/attribute.hpp"

namespace coro
{

using cond_type = std::function<bool()>;

class condition_variable;
using cond_var = condition_variable;

class condition_variable final
{
public:
    struct cv_awaiter : public mutex::mutex_awaiter
    {
        friend condition_variable;

        cv_awaiter(context& ctx, mutex& mtx, cond_var& cv) noexcept
            : mutex_awaiter(ctx, mtx),
              m_cv(cv)
        {
        }
        cv_awaiter(context& ctx, mutex& mtx, cond_var& cv, cond_type& cond) noexcept
            : mutex_awaiter(ctx, mtx),
              m_cv(cv),
              m_cond(cond)
        {
        }

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
            m_await_coro = handle;
            return register_lock();
        }

    protected:
        bool register_lock() noexcept;

        void register_cv() noexcept;

        void wake_up() noexcept;

        void resume() noexcept override;

        cond_type m_cond;
        cond_var& m_cv;
    };

public:
    condition_variable() noexcept = default;
    CORO_NO_COPY_MOVE(condition_variable);

    auto wait(mutex& mtx) noexcept -> cv_awaiter;

    auto wait(mutex& mtx, cond_type&& cond) noexcept -> cv_awaiter;

    auto wait(mutex& mtx, cond_type& cond) noexcept -> cv_awaiter;

    void notify_one() noexcept;

    void notify_all() noexcept;

private:
    // Spinlock lock_; // maybe unused
    cv_awaiter* m_head{nullptr};
    cv_awaiter* m_tail{nullptr};
};

}; // namespace coro
