#include <queue>

#include "coro/coro.hpp"

using namespace coro;

#define ITEM_NUM 2

cond_var        cv;
mutex           mtx;
std::queue<int> que;

task<> consumer()
{
    // utils::sleep(3);
    int  cnt{0};
    auto lock = co_await mtx.lock_guard();
    log::info("consumer hold lock");
    while (cnt < ITEM_NUM)
    {
        co_await cv.wait(mtx, [&]() { return !que.empty(); });
        while (!que.empty())
        {
            log::info("consumer fetch value: {}", que.front());
            que.pop();
            cnt++;
        }
        log::info("consumer cnt: {}", cnt);
        cv.notify_one();
    }
    log::info("consumer finish");
}

task<> producer()
{
    for (int i = 0; i < ITEM_NUM; i++)
    {
        auto lock = co_await mtx.lock_guard();
        log::info("producer hold lock");
        co_await cv.wait(mtx, [&]() { return que.empty(); });
        que.push(i);
        log::info("producer add value finish");
        cv.notify_one();
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(consumer());
    submit_to_scheduler(producer());

    scheduler::start();

    scheduler::loop(false);
    return 0;
}
