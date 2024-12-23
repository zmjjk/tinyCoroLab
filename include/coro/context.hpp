#pragma once

#include <memory>
#include <thread>
#include <atomic>

#include "config.hpp"
#include "coro/worker.hpp"
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

  class Context
  {
  public:
    Context() noexcept = default;
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

  public:
    void start() noexcept;

    void stop() noexcept;

    void join() noexcept;

    template <typename T>
    void submit_task(Task<T> &&task) noexcept
    {
      submit_task(task);
    }

    template <typename T>
    void submit_task(Task<T> &task) noexcept
    {
      worker_.submit_task(task.get_handler());
    }

    void submit_task(coroutine_handle<> handle) noexcept
    {
      worker_.submit_task(handle);
    }

    inline ctx_id get_ctx_id() noexcept
    {
      return id_;
    }

    inline void register_wait_task() noexcept
    {
      wait_task_++;
    }

    inline void unregister_wait_task() noexcept
    {
      wait_task_--;
    }

    inline bool empty_wait_task() noexcept
    {
      return wait_task_.load(memory_order_relaxed) == 0;
    }

    inline Worker &get_worker() noexcept
    {
      return worker_;
    }

  private:
    void init() noexcept;

    void deinit() noexcept;

    void run(stop_token token) noexcept;

    void process_work() noexcept;

    void poll_submit() noexcept;

  private:
    alignas(config::kCacheLineSize) Worker worker_;
    unique_ptr<jthread> job_;
    ctx_id id_;
    atomic<size_t> wait_task_{0};
  };

  Context *local_thread_context() noexcept;

  template <typename T>
  void submit_task(Task<T> &task) noexcept
  {
    local_thread_context()->submit_task(task.get_handler());
  }

  template <typename T>
  void submit_task(Task<T> &&task) noexcept
  {
    local_thread_context()->submit_task(task.get_handler());
  }

  inline void submit_task(coroutine_handle<> handle) noexcept
  {
    local_thread_context()->submit_task(handle);
  }
};