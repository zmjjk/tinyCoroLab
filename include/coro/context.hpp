#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "config.h"
#include "coro/engine.hpp"
#include "coro/log.hpp"
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

class context
{
public:
    context() noexcept { init(); }
    ~context() noexcept { deinit(); }
    context(const context&)            = delete;
    context(context&&)                 = delete;
    context& operator=(const context&) = delete;
    context& operator=(context&&)      = delete;

    auto start() noexcept -> void;

    auto stop() noexcept -> void;

    inline auto join() noexcept -> void { m_job->join(); }

    template<typename T>
    inline auto submit_task(task<T>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    template<typename T>
    inline auto submit_task(task<T>& task) noexcept -> void
    {
        m_engine.submit_task(task.handle());
    }

    inline auto submit_task(std::coroutine_handle<> handle) noexcept -> void { m_engine.submit_task(handle); }

    inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

    inline auto register_wait_task(bool registered) noexcept -> void
    {
        m_num_wait_task.fetch_add(int(registered), memory_order_relaxed);
    }

    inline auto unregister_wait_task() noexcept -> void { m_num_wait_task.fetch_sub(1, memory_order_relaxed); }

private:
    auto init() noexcept -> void;

    auto deinit() noexcept -> void;

    auto run(stop_token token) noexcept -> void;

    auto process_work() noexcept -> void;

    inline auto poll_work() noexcept -> void { m_engine.poll_submit(); }

    inline auto empty_wait_task() noexcept -> bool { return m_num_wait_task.load(memory_order_relaxed) == 0; }

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

template<typename T>
inline void submit_task(task<T>&& task) noexcept
{
    auto handle = task.handle();
    task.detach();
    submit_task(handle);
}

template<typename T>
inline void submit_task(task<T>& task) noexcept
{
    submit_task(task.handle());
}

inline void submit_task(std::coroutine_handle<> handle) noexcept
{
    local_context().submit_task(handle);
}

}; // namespace coro
