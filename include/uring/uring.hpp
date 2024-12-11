#pragma once

#include <cassert>
#include <liburing.h>
#include "config/config.hpp"

namespace coro
{
  using ursptr = io_uring_sqe *;
  using urcptr = io_uring_cqe *;

  class UringProxy
  {
  public:
    UringProxy() noexcept
    {
      init(config::kEntryLength);
    }

    ~UringProxy()
    {
      deinit();
    }

  private:
    void init(unsigned int entry_length)
    {
      int res = io_uring_queue_init(entry_length, &ring_, 0);
      assert(res == 0);
    }

    void deinit()
    {
      io_uring_queue_exit(&ring_);
    }

  private:
    [[maybe_unused]] io_uring_params para_;
    alignas(config::kCacheLineSize) io_uring ring_;
  };
};
