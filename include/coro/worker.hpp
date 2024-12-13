#pragma once

#include <array>
#include <queue>
#include <coroutine>
#include <functional>

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
    Worker() noexcept : num_task_wait_submit_(0), num_task_running_(0) {};
    ~Worker() noexcept {};

    void init()
    {
      urpxy_.init(config::kEntryLength);
    }

    void deinit()
    {
      urpxy_.deinit();
    }

    inline bool has_task_ready() noexcept
    {
      return !rcur_.isEmpty();
    }

    ursptr get_free_urs() noexcept
    {
      return urpxy_.get_free_sqe();
    }

    size_t num_task_schedule() noexcept
    {
      return rcur_.size();
    }

    coroutine_handle<> schedule() noexcept
    {
      assert(!rcur_.isEmpty());
      auto coro = rbuf_[rcur_.head()];
      rcur_.pop();
      assert(bool(coro));
      return coro;
    }

    void exec_one_task() noexcept
    {
      auto coro = schedule();
      coro.resume();
    }

    bool peek_uring()
    {
      return urpxy_.peek_uring();
    }

    void wait_uring(int num = 1)
    {
      urpxy_.wait_uring(num);
    }

    void handle_cqe_entry(urcptr cqe)
    {
      num_task_running_--;
      // TODO:
    }

    void poll_submit()
    {
      if (num_task_wait_submit_ > 0)
      {
        int num = urpxy_.submit();
        num_task_wait_submit_ -= num;
        assert(num_task_wait_submit_ == 0);
        num_task_running_ += num;
      }

      if (peek_uring())
      {
        [[__attribute_maybe_unused__]]
        auto num = urpxy_.handle_for_each_cqe([this](urcptr cqe)
                                              { this->handle_cqe_entry(cqe); });
      }
      else
      {
        wait_uring();
      }
    }

  private:
    alignas(config::kCacheLineSize) UringProxy urpxy_;
    std::queue<std::coroutine_handle<>> task_que_;
    RingCursor<size_t, config::kQueCap> rcur_;
    array<coroutine_handle<>, config::kQueCap> rbuf_;
    size_t num_task_wait_submit_;
    size_t num_task_running_;
  };
};
