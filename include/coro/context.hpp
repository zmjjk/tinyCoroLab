#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "config.h"
#include "coro/engine.hpp"
#include "coro/meta_info.hpp"
#include "coro/task.hpp"

namespace coro
{
using config::ctx_id;
using std::atomic;
using std::jthread;
using std::make_unique;
using std::memory_order_relaxed;
using std::stop_token;
using std::unique_ptr;

using detail::ginfo;
using detail::linfo;

using engine = detail::engine;

class scheduler;

class context
{
public:
    context() noexcept;
    ~context() noexcept                = default;
    context(const context&)            = delete;
    context(context&&)                 = delete;
    context& operator=(const context&) = delete;
    context& operator=(context&&)      = delete;

    auto start() noexcept -> void;

    auto stop() noexcept -> void;

    inline auto join() noexcept -> void { m_job->join(); }

    inline auto submit_task(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    inline auto submit_task(task<void>& task) noexcept -> void { m_engine.submit_task(task.handle()); }

    inline auto submit_task(std::coroutine_handle<> handle) noexcept -> void { m_engine.submit_task(handle); }

    inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

    inline auto register_wait(bool register_flag = true) noexcept -> void CORO_INLINE
    {
        m_num_wait_task.fetch_add(size_t(register_flag), memory_order_relaxed);
    }

    inline auto unregister_wait() noexcept -> void CORO_INLINE { m_num_wait_task.fetch_sub(1, memory_order_relaxed); }

    inline auto get_engine() noexcept -> engine& { return m_engine; }

private:
    auto init() noexcept -> void;

    auto deinit() noexcept -> void;

    auto run(stop_token token) noexcept -> void;

    auto process_work() noexcept -> void;

    inline auto poll_work() noexcept -> void { m_engine.poll_submit(); }

    inline auto empty_wait_task() noexcept -> bool CORO_INLINE
    {
        return m_num_wait_task.load(memory_order_relaxed) == 0 && m_engine.empty_io();
    }

private:
    CORO_ALIGN engine   m_engine;
    unique_ptr<jthread> m_job;
    ctx_id              m_id;
    atomic<size_t>      m_num_wait_task{0};
};

inline context& local_context() noexcept
{
    return *linfo.ctx;
}

inline void submit_to_context(task<void>&& task) noexcept
{
    local_context().submit_task(std::move(task));
}

inline void submit_to_context(task<void>& task) noexcept
{
    local_context().submit_task(task.handle());
}

inline void submit_to_context(std::coroutine_handle<> handle) noexcept
{
    local_context().submit_task(handle);
}

}; // namespace coro
