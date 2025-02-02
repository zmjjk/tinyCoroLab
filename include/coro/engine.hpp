#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/uring_proxy.hpp"

namespace coro
{
using std::array;
using std::atomic;
using std::coroutine_handle;
using std::memory_order_relaxed;
using std::queue;
using uring::urcptr;
using uring::uring_proxy;
using uring::ursptr;

template<typename T>
// single producer and single consumer
using spsc_queue = AtomicQueue<T>;

class engine
{
public:
    engine() noexcept : m_num_io_wait_submit(0), m_num_io_running(0) {}
    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

    void init() noexcept
    {
        m_upxy.init(config::kEntryLength);
        m_num_io_wait_submit = 0;
        m_num_io_running     = 0;

        spsc_queue<coroutine_handle<>> task_queue;
        m_task_queue.swap(task_queue);
    }

    void deinit() noexcept { m_upxy.deinit(); }

    inline bool ready() noexcept { return !m_task_queue.was_empty(); }

    inline ursptr get_free_urs() noexcept { return m_upxy.get_free_sqe(); }

    inline size_t num_task_schedule() noexcept { return m_task_queue.was_size(); }

    coroutine_handle<> schedule() noexcept
    {
        auto coro = m_task_queue.pop();
        assert(bool(coro));
        return coro;
    }

    void submit_task(coroutine_handle<> handle) noexcept
    {
        m_task_queue.push(handle);
        wake_up();
    }

    void exec_one_task() noexcept
    {
        auto coro = schedule();
        coro.resume();
    }

    inline bool peek() noexcept { return m_upxy.peek_uring(); }

    inline void wait(int num = 1) noexcept { return m_upxy.wait_uring(num); }

    void handle_cqe_entry(urcptr cqe) noexcept;

    void poll_submit() noexcept
    {
        int num_task_wait = m_num_io_wait_submit.load(memory_order_relaxed);
        if (num_task_wait > 0)
        {
            int num = m_upxy.submit();
            num_task_wait -= num;
            assert(num_task_wait == 0);
            m_num_io_wait_submit.fetch_sub(num, memory_order_relaxed);
        }

        auto cnt = m_upxy.wait_eventfd();

        auto uringnum = GETURINGNUM(cnt);

        auto num = m_upxy.peek_batch_cqe(m_urc.data(), uringnum);

        assert(num == uringnum);
        for (int i = 0; i < num; i++)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
    }

    void wake_up() noexcept { m_upxy.write_eventfd(SETTASKNUM); }

    inline void add_wait_task() noexcept { m_num_io_wait_submit.fetch_add(1, memory_order_relaxed); }

private:
    uring_proxy                    m_upxy;
    spsc_queue<coroutine_handle<>> m_task_queue;
    array<urcptr, config::kQueCap> m_urc;
    atomic<size_t>                 m_num_io_wait_submit;
    size_t                         m_num_io_running;
};
}; // namespace coro