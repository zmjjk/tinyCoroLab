#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 5

event<> ev;

task<> set_task()
{
    event_guard guard(ev);
    log::info("set task ready to sleep");
    utils::sleep(3);
    log::info("ready to set event");
    co_return;
}

task<> wait_task(int i)
{
    co_await ev.wait();
    log::info("task {} wake up by event", i);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[CONTEXTNUM];
    for (int i = 0; i < CONTEXTNUM - 1; i++)
    {
        ctx[i].submit_task(wait_task(i));
    }
    ctx[CONTEXTNUM - 1].submit_task(set_task());

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
