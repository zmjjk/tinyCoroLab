#pragma once

#include <array>
#include <queue>
#include <coroutine>
#include <functional>
#include <atomic>

#include "uring/uring.hpp"
#include "uds/ring_cursor.hpp"
#include "config.hpp"

namespace coro
{
  using ds::RingCursor;
  using std::array;
  using std::atomic;
  using std::coroutine_handle;
  using std::memory_order_relaxed;
  using std::queue;

  class Worker
  {
  public:
    Worker() noexcept : num_task_wait_submit_(0), num_task_running_(0) {};
    ~Worker() noexcept {};

    void init() noexcept;

    void deinit() noexcept;

    inline bool has_task_ready() noexcept;

    inline ursptr get_free_urs() noexcept
    {
      return urpxy_.get_free_sqe();
    }

    inline size_t num_task_schedule() noexcept { return rcur_.size(); }

    coroutine_handle<> schedule() noexcept;

    void submit_task(coroutine_handle<> handle) noexcept;

    void exec_one_task() noexcept;

    inline bool peek_uring() noexcept;

    inline void wait_uring(int num = 1) noexcept;

    void handle_cqe_entry(urcptr cqe) noexcept;

    void poll_submit() noexcept;

    void wake_up() noexcept;

    inline void add_wait_task() noexcept
    {
      num_task_wait_submit_.fetch_add(1, memory_order_relaxed);
    }

  private:
    alignas(config::kCacheLineSize) UringProxy urpxy_;
    std::queue<std::coroutine_handle<>> task_que_;
    RingCursor<size_t, config::kQueCap> rcur_;
    array<coroutine_handle<>, config::kQueCap> rbuf_;
    array<urcptr, config::kQueCap> cqe_;
    atomic<size_t> num_task_wait_submit_;
    size_t num_task_running_;
  };
};
