#pragma once

#include <memory>
#include <thread>
#include <iostream>

#include "config.hpp"
#include "coro/worker.hpp"
#include "coro/task.hpp"

namespace coro
{
  using config::ctx_id;
  using std::jthread;
  using std::make_unique;
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
  };

  Context *local_thread_context() noexcept;
};