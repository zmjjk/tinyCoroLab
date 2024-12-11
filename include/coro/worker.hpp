#pragma once

#include <array>
#include <queue>
#include <coroutine>

#include "uring/uring.hpp"
#include "uds/ring_cursor.hpp"
#include "config/config.hpp"

namespace coro
{
  using ds::RingCursor;
  using std::array;
  using std::coroutine_handle;
  using std::queue;

  class Worker
  {
  public:
    Worker() noexcept {};
    ~Worker() noexcept {};

    ursptr getFreeUrs() noexcept {}

    size_t numTaskToSchedule() noexcept
    {
      return rcur_.size();
    }

  private:
    alignas(config::kCacheLineSize) UringProxy urpxy_;
    std::queue<std::coroutine_handle<>> task_que_;
    RingCursor<size_t, config::kQueCap> rcur_;
    array<coroutine_handle<>, config::kQueCap> buf_;
  };
};
