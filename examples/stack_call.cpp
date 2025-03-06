#include "coro/coro.hpp"

using namespace coro;

task<int> add(int x, int y)
{
    co_return x + y;
}

task<> func(int x, int y)
{
    auto result = co_await add(x, y);
    log::info("func result: {}", result);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    scheduler::submit(func(3, 4));

    scheduler::start();

    scheduler::stop();
    return 0;
}
