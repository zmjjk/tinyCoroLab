#pragma once

#include <memory>
#include <thread>

#include "config/config.hpp"
#include "coro/worker.hpp"
#include "coro/task.hpp"

namespace coro
{
  using std::jthread;
  using std::make_unique;
  using std::stop_token;
  using std::unique_ptr;

  class Context
  {
  public:
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

  public:
    void start()
    {
      job_ = make_unique<jthread>([this](stop_token token)
                                  {
        this->init();
        this->run(token);
        this->deinit(); });
    }

    void stop()
    {
      job_->request_stop();
    }

    template <typename T>
    void submit_task(Task<T> &&task)
    {
      submit_task(task);
    }

    template <typename T>
    void submit_task(Task<T> &task)
    {
    }

  private:
    void init() { worker_.init(); }

    void deinit() { worker_.deinit(); }

    void run(stop_token token)
    {
      while (!token.stop_requested())
      {
        // TODO: work
        process_work();
        poll_submit();
      }
    }

    void process_work()
    {
      auto num = worker_.num_task_schedule();
      for (int i = 0; i < num; i++)
      {
        worker_.exec_one_task();
      }
    }

    void poll_submit()
    {
      worker_.poll_submit();
    }

  private:
    alignas(config::kCacheLineSize) Worker worker_;
    unique_ptr<jthread> job_;
  };
};