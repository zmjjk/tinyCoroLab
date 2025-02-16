#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/attribute.hpp"
#include "coro/uring_proxy.hpp"

namespace coro
{
class context;
};

namespace coro::detail
{
using std::array;
using std::atomic;
using std::coroutine_handle;
using std::queue;
using uring::urcptr;
using uring::uring_proxy;
using uring::ursptr;

template<typename T>
// single producer and single consumer
using spsc_queue = AtomicQueue<T>;

class engine
{
    friend class ::coro::context;

public:
    engine() noexcept : m_num_io_wait_submit(0), m_num_io_running(0) {}
    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

private:
    auto init() noexcept -> void;

    auto deinit() noexcept -> void;

    inline auto ready() noexcept -> bool { return !m_task_queue.was_empty(); }

    [[CORO_DISCARD_HINT]] inline auto get_free_urs() noexcept -> ursptr { return m_upxy.get_free_sqe(); }

    inline auto num_task_schedule() noexcept -> size_t { return m_task_queue.was_size(); }

    [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

    auto submit_task(coroutine_handle<> handle) noexcept -> void;

    auto exec_one_task() noexcept -> void;

    auto handle_cqe_entry(urcptr cqe) noexcept -> void;

    auto poll_submit() noexcept -> void;

    auto wake_up() noexcept -> void;

    inline auto add_wait_task() noexcept -> void { m_num_io_wait_submit.fetch_add(1, std::memory_order_relaxed); }

private:
    uring_proxy                    m_upxy;
    spsc_queue<coroutine_handle<>> m_task_queue;
    array<urcptr, config::kQueCap> m_urc;
    atomic<size_t>                 m_num_io_wait_submit;
    size_t                         m_num_io_running;
};
}; // namespace coro::detail
