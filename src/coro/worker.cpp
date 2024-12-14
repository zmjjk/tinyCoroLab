#include "coro/worker.hpp"

namespace coro
{
  void Worker::init()
  {
    urpxy_.init(config::kEntryLength);
  }

  void Worker::deinit()
  {
    urpxy_.deinit();
  }

  bool Worker::has_task_ready()
  {
    return !rcur_.isEmpty();
  }

  ursptr Worker::get_free_urs()
  {
    return urpxy_.get_free_sqe();
  }

  size_t Worker::num_task_schedule()
  {
    return rcur_.size();
  }

  coroutine_handle<> Worker::schedule()
  {
    assert(!rcur_.isEmpty());
    auto coro = rbuf_[rcur_.head()];
    rcur_.pop();
    assert(bool(coro));
    return coro;
  }

  void Worker::submit_task(coroutine_handle<> handle)
  {
    rbuf_[rcur_.tail()] = handle;
    rcur_.push();
  }

  void Worker::exec_one_task()
  {
    auto coro = schedule();
    coro.resume();
  }

  bool Worker::peek_uring()
  {
    return urpxy_.peek_uring();
  }

  void Worker::wait_uring(int num = 1)
  {
    urpxy_.wait_uring(num);
  }

  void Worker::handle_cqe_entry(urcptr cqe)
  {
    num_task_running_--;
    // TODO: process
  }

  void Worker::poll_submit()
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
};