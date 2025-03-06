#include "coro/coro.hpp"

using namespace coro;

task<> sub_func(int i)
{
    log::info("sub func {} running...", i);
    co_return;
}

task<> func(int i)
{
    log::info("ready to wait all sub task");
    co_await when_all(sub_func(i + 1), sub_func(i + 2), sub_func(i + 3));
    log::info("wait all sub task finish");
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    scheduler::submit(func(0));

    scheduler::start();
    scheduler::stop();
    return 0;
}
