#include "coro/context.hpp"
#include "lock/mutex.hpp"

using namespace coro;

Mutex mtx;
uint64_t data = 0;

Task<> run()
{
  co_await mtx.lock_guard();
  for (int i = 0; i < 10000; i++)
  {
    data++;
  }
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx[10];
  for (int i = 0; i < 64; i++)
  {
    ctx[i].submit_task(run());
  }

  for (int i = 0; i < 64; i++)
  {
    ctx[i].start();
  }

  return 0;
}
