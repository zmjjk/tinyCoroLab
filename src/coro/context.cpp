#include "coro/context.hpp"
#include "coro/thread_info.hpp"

#include <iostream>

namespace coro
{
  void Context::start() noexcept
  {
    job_ = make_unique<jthread>([this](stop_token token)
                                {
        this->init();
        this->run(token);
        this->deinit(); });
  }

  void Context::stop() noexcept
  {
    job_->request_stop();
    // job_->join();
  }

  void Context::init() noexcept
  {
    id_ = global_info.context_num++;
    thread_info.context = this;
    worker_.init();
  }

  void Context::deinit() noexcept
  {
    thread_info.context = nullptr;
    worker_.deinit();
  }

  void Context::run(stop_token token) noexcept
  {
    while (!token.stop_requested())
    {
      process_work();
      poll_submit();
    }
    process_work();
  }

  void Context::process_work() noexcept
  {
    auto num = worker_.num_task_schedule();
    for (int i = 0; i < num; i++)
    {
      worker_.exec_one_task();
    }
  }

  void Context::poll_submit() noexcept
  {
    worker_.poll_submit();
  }

  Context *local_thread_context() noexcept
  {
    return thread_info.context;
  }
};