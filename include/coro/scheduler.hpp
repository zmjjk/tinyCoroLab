#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "coro/dispatcher.hpp"

#include "coro/log.hpp"

namespace coro
{

class scheduler
{
    friend context;

public:
    inline static auto init(size_t ctx_cnt = std::thread::hardware_concurrency()) noexcept -> void
    {
        get_instance()->init_impl(ctx_cnt);
    }

    inline static auto start() noexcept -> void { get_instance()->start_impl(); }

    inline static auto stop() noexcept -> void { get_instance()->stop_impl(); }

    inline static auto join() noexcept -> void { get_instance()->join_impl(); }

    static inline auto submit(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        submit(handle);
    }

    static inline auto submit(task<void>& task) noexcept -> void { submit(task.handle()); }

    inline static auto submit(std::coroutine_handle<> handle) noexcept -> void
    {
        get_instance()->submit_task_impl(handle);
    }

private:
    static auto get_instance() noexcept -> scheduler*
    {
        static scheduler sc;
        return &sc;
    }

    auto init_impl(size_t ctx_cnt) noexcept -> void;

    auto start_impl() noexcept -> void;

    auto stop_impl() noexcept -> void;

    auto join_impl() noexcept -> void;

    auto submit_task_impl(std::coroutine_handle<> handle) noexcept -> void;

private:
    size_t                                              m_ctx_cnt{0};
    detail::ctx_container                               m_ctxs;
    detail::dispatcher<coro::config::kDispatchStrategy> m_dispatcher;
};

inline void submit_to_scheduler(task<void>&& task) noexcept
{
    scheduler::submit(std::move(task));
}

inline void submit_to_scheduler(task<void>& task) noexcept
{
    scheduler::submit(task.handle());
}

inline void submit_to_scheduler(std::coroutine_handle<> handle) noexcept
{
    scheduler::submit(handle);
}

}; // namespace coro
