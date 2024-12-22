#include "coro/worker.hpp"
#include "net/io_info.hpp"
#include "log/log.hpp"

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
    wake_up();
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
    auto data = reinterpret_cast<IoInfo *>(io_uring_cqe_get_data(cqe));
    if (data == nullptr)
    {
      log::info("error when fetch data, res: {}, data: {}", cqe->res, cqe->user_data);
    }
    else
    {
      log::info("normal");
    }

    switch (data->type)
    {
    case Tasktype::Accept:
      data->result = cqe->res;
      log::info("receive accept task");
      submit_task(data->handle);
      break;
    default:
      break;
    }
  }

  void Worker::poll_submit() noexcept
  {
    int num_task_wait = num_task_wait_submit_.load(memory_order_relaxed);
    if (num_task_wait > 0)
    {
      int num = urpxy_.submit();
      num_task_wait -= num;
      assert(num_task_wait == 0);
      num_task_wait_submit_.fetch_sub(num, memory_order_relaxed);
    }

    auto cnt = urpxy_.wait_eventfd();

    auto uringnum = GETURINGNUM(cnt);

    auto num = urpxy_.peek_batch_cqe(cqe_.data(), uringnum);

    assert(num == uringnum);
    for (int i = 0; i < num; i++)
    {
      handle_cqe_entry(cqe_[i]);
    }
    urpxy_.cq_advance(num);

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