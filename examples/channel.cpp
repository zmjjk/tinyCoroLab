#include "coro/coro.hpp"

using namespace coro;

channel<int> ch;

task<> producer(int i)
{
    for (int i = 0; i < 10; i++)
    {
        co_await ch.send(i);
    }

    utils::sleep(1);
    ch.close();
    co_return;
}

task<> consumer(int i)
{
    while (true)
    {
        auto data = co_await ch.recv();
        if (data)
        {
            log::info("consumer {} receive data: {}", i, *data);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[2];
    ctx[0].submit_task(producer(0));
    ctx[1].submit_task(consumer(0));

    ctx[0].start();
    ctx[1].start();

    ctx[0].stop();
    ctx[1].stop();
    return 0;
}
