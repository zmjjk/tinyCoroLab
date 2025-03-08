#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 10

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
    scheduler::init();

    for (int i = 0; i < TASK_NUM; i++)
    {
        submit_to_scheduler(run(i));
    }
    submit_to_scheduler(run2());

    scheduler::start();

    scheduler::stop();
    return 0;
}
