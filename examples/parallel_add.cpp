#include <vector>

#include "lock/condition_variable.hpp"
#include "coro/context.hpp"
#include "log/log.hpp"

using namespace coro;

#define CONTEXTNUM 5

std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// TODO: how to collect result
Task<int> calc(int lef, int rig)
{
  int sum = 0;
  for (int i = lef; i < rig; i++)
  {
    sum += vec[i];
  }
  co_return sum;
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx[CONTEXTNUM];
  for (int i = 0; i < CONTEXTNUM; i++)
  {
    ctx[i].submit_task(calc(i * 3, (i + 1) * 3));
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
    log::info("context {} stop finish", i);
  }
  return 0;
}
