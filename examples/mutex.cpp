#include <iostream>
#include <chrono>

#include "coro/context.hpp"
#include "lock/mutex.hpp"
#include "log/log.hpp"

using namespace coro;

#define CONTEXTNUM 100

Mutex mtx;
uint64_t data = 0;

Task<> run()
{
  auto cid = thread_info.context->get_ctx_id();
  auto lock = co_await mtx.lock_guard();
  for (int i = 0; i < 100000; i++)
  {
    data++;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  log::info("context {} task finish run, data: {}",
            cid, data);
  co_return;
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx[CONTEXTNUM];
  for (int i = 0; i < CONTEXTNUM; i++)
  {
    ctx[i].submit_task(run());
    log::info("context {} submit task", i);
  }

  for (int i = 0; i < CONTEXTNUM; i++)
  {
    ctx[i].start();
    log::info("context {} start task", i);
  }

  for (int i = 0; i < CONTEXTNUM; i++)
  {
    ctx[i].stop();
  }

  return 0;
}
