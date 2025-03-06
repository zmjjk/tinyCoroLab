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
    scheduler::init();

    scheduler::submit(producer(0));
    scheduler::submit(producer(1));
    scheduler::submit(consumer(2));
    scheduler::submit(consumer(3));

    scheduler::start();

    utils::sleep(1);
    ch.close();

    scheduler::stop();
    return 0;
}
