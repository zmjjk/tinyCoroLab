#include "coro/worker.hpp"

#include <iostream>

namespace coro
{
  void Worker::init() noexcept
  {
    urpxy_.init(config::kEntryLength);
  }

  void Worker::deinit() noexcept
  {
    urpxy_.deinit();
  }

  bool Worker::has_task_ready() noexcept
  {
    return !rcur_.isEmpty();
  }

  ursptr Worker::get_free_urs() noexcept
  {
    return urpxy_.get_free_sqe();
  }

  coroutine_handle<> Worker::schedule() noexcept
  {
    assert(!rcur_.isEmpty());
    auto coro = rbuf_[rcur_.head()];
    rcur_.pop();
    assert(bool(coro));
    return coro;
  }

  void Worker::submit_task(coroutine_handle<> handle) noexcept
  {
    rbuf_[rcur_.tail()] = handle;
    rcur_.push();
    urpxy_.write_eventfd(SETTASKNUM);
  }

  void Worker::exec_one_task() noexcept
  {
    auto coro = schedule();
    coro.resume();
  }

  bool Worker::peek_uring() noexcept
  {
    return urpxy_.peek_uring();
  }

  void Worker::wait_uring(int num) noexcept
  {
    urpxy_.wait_uring(num);
  }

  void Worker::handle_cqe_entry(urcptr cqe) noexcept
  {
    num_task_running_--;
    // TODO: process
  }

  void Worker::poll_submit() noexcept
  {
    if (num_task_wait_submit_ > 0)
    {
      int num = urpxy_.submit();
      num_task_wait_submit_ -= num;
      assert(num_task_wait_submit_ == 0);
      num_task_running_ += num;
    }

    auto cnt = urpxy_.wait_eventfd();

    auto uringnum = GETURINGNUM(cnt);
    auto num = urpxy_.peek_batch_cqe(cqe_.data(), uringnum);
    assert(num == uringnum);
    for (int i = 0; i < num; i++)
    {
      handle_cqe_entry(cqe_[i]);
      // TODO: use io_uring_cq_advance
    }

    // if (peek_uring())
    // {
    //   [[__attribute_maybe_unused__]]
    //   auto num = urpxy_.handle_for_each_cqe([this](urcptr cqe)
    //                                         { this->handle_cqe_entry(cqe); });
    // }
    // else
    // {
    //   wait_uring();
    // }
  }

  void Worker::wake_up() noexcept
  {
    urpxy_.write_eventfd(SETTASKNUM);
  }
};