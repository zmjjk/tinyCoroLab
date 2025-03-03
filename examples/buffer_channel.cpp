#include "coro/coro.hpp"

using namespace coro;

channel<int, 5> ch;

task<> producer(int id)
{
    for (int i = 0; i < 4; i++)
    {
        co_await ch.send(id * 10 + i);
        log::info("producer {} send once", id);
    }

    co_return;
}

task<> consumer(int id)
{
    while (true)
    {
        auto data = co_await ch.recv();
        if (data)
        {
            log::info("consumer {} receive data: {}", id, *data);
        }
        else
        {
            log::info("consumer {} receive close", id);
            break;
        }
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    context ctx[4];
    ctx[0].submit_task(producer(0));
    ctx[1].submit_task(producer(1));
    ctx[2].submit_task(consumer(2));
    ctx[3].submit_task(consumer(3));

    ctx[0].start();
    ctx[1].start();
    ctx[2].start();
    ctx[3].start();

    utils::sleep(1);
    ch.close();

    ctx[0].stop();
    ctx[1].stop();
    ctx[2].stop();
    ctx[3].stop();
    return 0;
}
