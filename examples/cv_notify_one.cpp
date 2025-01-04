#include <thread>
#include <chrono>
#include <queue>

#include "lock/condition_variable.hpp"
#include "coro/context.hpp"
#include "log/log.hpp"

using namespace coro;

CondVar cv;
Mutex mtx;
std::queue<int> que;

Task<> consumer()
{
  int cnt{0};
  auto lock = co_await mtx.lock_guard();
  log::info("consumer hold lock");
  while (cnt < 100)
  {
    co_await cv.wait(mtx, [&]()
                     { return !que.empty(); });
    while (!que.empty())
    {
      log::info("consumer fetch value: {}", que.front());
      que.pop();
      cnt++;
    }
    log::info("consumer cnt: {}", cnt);
    cv.notify_one();
  }
  log::info("consumer finish");
}

Task<> producer()
{
  for (int i = 0; i < 10; i++)
  {
    auto lock = co_await mtx.lock_guard();
    log::info("producer hold lock");
    co_await cv.wait(mtx, [&]()
                     { return que.size() < 10; });
    for (int j = 0; j < 10; j++)
    {
      que.push(i * 10 + j);
    }
    log::info("producer add value finish");
    cv.notify_one();
  }
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx[2];
  ctx[0].submit_task(consumer());
  log::info("context 0 submit consumer task");

  ctx[1].submit_task(producer());
  log::info("context 1 submit producer task");

  ctx[0].start();
  log::info("context 0 start task");

  ctx[1].start();
  log::info("context 1 start task");

  ctx[0].stop();
  log::info("context 0 stop finish");
  ctx[1].stop();
  log::info("context 1 stop finish");
  return 0;
}
