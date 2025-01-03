#pragma once

#include <coroutine>

#include "net/io_info.hpp"
#include "uring/uring.hpp"

#include "coro/context.hpp"

namespace coro::io
{
  using std::coroutine_handle;

  class IoAwaiter
  {
  public:
    IoAwaiter() noexcept : sqe_(local_thread_context()->get_worker().get_free_urs())
    {
      // io_uring_sqe_set_data(sqe_, &info_);
    }

    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      info_.handle = handle;
    }

    int32_t await_resume() noexcept { return info_.result; }

    // inline uintptr_t get_data() noexcept { return info_.data; }

  protected:
    IoInfo info_;
    ursptr sqe_;
  };
};