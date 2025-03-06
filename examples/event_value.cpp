#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 5

event<int> ev;

task<> set_task(int i)
{
    log::info("set task ready to sleep");
    utils::sleep(3);
    log::info("ready to set event");
    ev.set(i);
    co_return;
}

task<> wait_task(int i)
{
    auto val = co_await ev.wait();
    log::info("task {} wake up by event, get value: {}", i, val);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < TASK_NUM - 1; i++)
    {
        scheduler::submit(wait_task(i));
    }
    scheduler::submit(set_task(TASK_NUM - 1));

    scheduler::start();

    scheduler::stop();
    return 0;
}
