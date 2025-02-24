#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 10

cond_var cv;
mutex    mtx;

task<> run(int i)
{
    auto lock = co_await mtx.lock_guard();
    co_await cv.wait(mtx);
    log::info("task {} run finish", i);
    cv.notify_all();
}

task<> run2()
{
    utils::sleep(3);
    auto lock = co_await mtx.lock_guard();
    cv.notify_all();
    log::info("run2 task finish");
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[CONTEXTNUM + 1];
    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].submit_task(run(i));
        log::info("context {} submit task", i);
    }
    ctx[CONTEXTNUM].submit_task(run2());

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].start();
        log::info("context {} start task", i);
    }
    ctx[CONTEXTNUM].start();

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].stop();
        log::info("context {} stop finish", i);
    }
    ctx[CONTEXTNUM].stop();
    return 0;
}
