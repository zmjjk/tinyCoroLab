#include <mutex>
#include <vector>

#include "coro/coro.hpp"

using namespace coro;

const int task_num = 10000;

event            ev;
mutex            mtx;
std::vector<int> vec;
int              setid{0};
int              id{0};
bool             mark[task_num + 100] = {0};

task<> set_func_hybrid_mutex()
{
    auto guard = event_guard(ev);
    setid      = (++id);
    log::info("set finish");
    co_return;
}

task<> wait_func_hybrid_mutex(int i)
{
    co_await ev.wait();
    // log::info("id: {}", i);
    auto p = co_await mtx.lock_guard();
    // log::info("id: {}", i);
    vec.push_back((++id));
    mark[i] = true;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < task_num; i++)
    {
        submit_to_scheduler(wait_func_hybrid_mutex(i));
    }
    submit_to_scheduler(set_func_hybrid_mutex());

    scheduler::start();
    scheduler::loop(false);

    log::info("vector sz: {}", vec.size());
    for (int i = 0; i < task_num; i++)
    {
        if (!mark[i])
        {
            log::info("lose task {}", i);
        }
    }

    return 0;
}
