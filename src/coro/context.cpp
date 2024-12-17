#include "coro/context.hpp"
#include "coro/thread_info.hpp"

namespace coro
{
  void Context::start()
  {
    job_ = make_unique<jthread>([this](stop_token token)
                                {
        this->init();
        this->run(token);
        this->deinit(); });
  }

  void Context::stop()
  {
    job_->request_stop();
  }

  void Context::init()
  {
    thread_info.context = this;
    worker_.init();
  }

  void Context::deinit()
  {
    thread_info.context = nullptr;
    worker_.deinit();
  }

  void Context::run(stop_token token)
  {
    while (!token.stop_requested())
    {
      process_work();
      poll_submit();
    }
  }

  void Context::process_work()
  {
    auto num = worker_.num_task_schedule();
    for (int i = 0; i < num; i++)
    {
      worker_.exec_one_task();
    }
  }

  void Context::poll_submit()
  {
    worker_.poll_submit();
  }

  Context *local_thread_context() noexcept
  {
    return thread_info.context;
  }
};