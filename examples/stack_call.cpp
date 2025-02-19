#include "coro/coro.hpp"

using namespace coro;

task<int> add(int x, int y)
{
    co_return x + y;
}

task<void> func(int x, int y)
{
    auto result = co_await add(x, y);
    log::info("func result: {}", result);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx;
    ctx.submit_task(func(3, 4));
    log::info("context submit task finish");

    ctx.start();

    ctx.stop();

    log::info("context stop");

    return 0;
}
