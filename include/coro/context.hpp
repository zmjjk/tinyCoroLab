#pragma once

#include <memory>
#include <thread>

#include "config/config.hpp"
#include "coro/worker.hpp"
#include "coro/task.hpp"

namespace coro
{
  using std::jthread;
  using std::make_unique;
  using std::stop_token;
  using std::unique_ptr;

  class Context
  {
  public:
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

  public:
    void start() noexcept;

    void stop() noexcept;

    template <typename T>
    void submit_task(Task<T> &&task)
    {
      submit_task(task);
    }

    template <typename T>
    void submit_task(Task<T> &task)
    {
      worker_.submit_task(task.get_handler());
    }

    void submit_task(coroutine_handle<> handle)
    {
      worker_.submit_task(handle);
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
  };

  Context *local_thread_context() noexcept;
};