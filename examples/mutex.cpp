#include <iostream>

#include "coro/context.hpp"
#include "lock/mutex.hpp"
#include "log/log.hpp"

using namespace coro;

Mutex mtx;
uint64_t data = 0;

Task<> run()
{
  auto lock = co_await mtx.lock_guard();
  for (int i = 0; i < 10000; i++)
  {
    data++;
  }
  std::cout << "task finish, run: " << data << std::endl;
}

int main(int argc, char const *argv[])
{
  /* code */
  log::info("logtest: {}", 123);
  Context ctx[10];
  for (int i = 0; i < 3; i++)
  {
    ctx[i].submit_task(run());
    std::cout << "context " << i << " submit task" << std::endl;
  }

  for (int i = 0; i < 3; i++)
  {
    ctx[i].start();
    std::cout << "context " << i << " start task" << std::endl;
  }

  ctx[0].stop();

  return 0;
}
