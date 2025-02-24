#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 100

cond_var cv;
mutex    mtx;
uint64_t data = 0;
int      id   = 0;

task<> run(int i)
{
    auto lock = co_await mtx.lock_guard();
    co_await cv.wait(mtx, [&]() { return i == id; });
    for (int i = 0; i < 100000; i++)
    {
        data++;
    }
    log::info("task {} run finish, result: {}", i, data);
    id++;
    cv.notify_all();
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[CONTEXTNUM];
    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].submit_task(run(i));
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
