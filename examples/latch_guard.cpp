#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 5

latch l(CONTEXTNUM - 1);

task<> set_task(int i)
{
    latch_guard guard(l);
    log::info("set task ready to sleep");
    utils::sleep(2);
    co_return;
}

task<> wait_task(int i)
{
    co_await l.wait();
    log::info("task {} wake up by latch", i);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[CONTEXTNUM];

    ctx[CONTEXTNUM - 1].submit_task(wait_task(CONTEXTNUM - 1));
    for (int i = 0; i < CONTEXTNUM - 1; i++)
    {
        ctx[i].submit_task(set_task(i));
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].start();
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].stop();
    }

    return 0;
}
