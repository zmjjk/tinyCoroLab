#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 100

mutex mtx;
int   data = 0;

task<> add_task(int i)
{
    auto guard = co_await mtx.lock_guard();
    log::info("task {} fetch lock", i);
    for (int i = 0; i < 10000; i++)
    {
        data += 1;
    }
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[CONTEXTNUM];
    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].submit_task(add_task(i));
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].start();
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].stop();
    }
    assert(data == 1000000);
    return 0;
}
