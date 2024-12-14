#include "coro/context.hpp"

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

  void Context::init() { worker_.init(); }

  void Context::deinit() { worker_.deinit(); }

  void Context::run(stop_token token)
  {
    while (!token.stop_requested())
    {
      // TODO: work
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
};