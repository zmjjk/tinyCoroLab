#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/attribute.hpp"
#include "coro/meta_info.hpp"
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

#define wake_by_task(val) (((val) & engine::task_mask) > 0)
#define wake_by_io(val)   (((val) & engine::io_mask) > 0)
#define wake_by_cqe(val)  (((val) & engine::cqe_mask) > 0)

class engine
{
    friend class ::coro::context;

public:
    static constexpr uint64_t task_mask = (0xFFFFF00000000000);
    static constexpr uint64_t io_mask   = (0x00000FFFFF000000);
    static constexpr uint64_t cqe_mask  = (0x0000000000FFFFFF);

    static constexpr uint64_t task_flag = (((uint64_t)1) << 44);
    static constexpr uint64_t io_flag   = (((uint64_t)1) << 24);

    engine() noexcept : m_num_io_wait_submit(0), m_num_io_running(0)
    {
        m_id = ginfo.engine_id.fetch_add(1, std::memory_order_relaxed);
    }

    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

    // mark
    auto init() noexcept -> void;

    // mark
    auto deinit() noexcept -> void;

    // mark
    inline auto ready() noexcept -> bool { return !m_task_queue.was_empty(); }

    [[CORO_DISCARD_HINT]] inline auto get_free_urs() noexcept -> ursptr { return m_upxy.get_free_sqe(); }

    // mark
    inline auto num_task_schedule() noexcept -> size_t { return m_task_queue.was_size(); }

    // mark
    [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

    // mark
    auto submit_task(coroutine_handle<> handle) noexcept -> void;

    // mark
    auto exec_one_task() noexcept -> void;

    auto handle_cqe_entry(urcptr cqe) noexcept -> void;

    // mark
    auto poll_submit() noexcept -> void;

    auto wake_up(uint64_t val = engine::task_flag) noexcept -> void;

    // mark
    inline auto add_io_submit() noexcept -> void
    {
        m_num_io_wait_submit.fetch_add(1, std::memory_order_release);
        wake_up(io_flag);
    }

    // mark
    inline auto empty_io() noexcept -> bool
    {
        return m_num_io_wait_submit.load(std::memory_order_acquire) == 0 &&
               m_num_io_running.load(std::memory_order_acquire) == 0;
    }

    inline auto get_id() noexcept -> uint32_t { return m_id; }

private:
    auto do_io_submit() noexcept -> void;

private:
    uint32_t                       m_id;
    uring_proxy                    m_upxy;
    spsc_queue<coroutine_handle<>> m_task_queue;
    array<urcptr, config::kQueCap> m_urc;
    atomic<size_t>                 m_num_io_wait_submit{0};
    atomic<size_t>                 m_num_io_running{0};
};

inline engine& local_engine() noexcept
{
    return *linfo.egn;
}

}; // namespace coro::detail
