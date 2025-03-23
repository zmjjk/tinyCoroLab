#include <algorithm>
#include <latch>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename waitgroup_type>
void waitgroup_bench(const int loop_num);

task<> done(std::latch& lt, const int loop_num)
{
    loop_add;
    lt.count_down();
    co_return;
}

task<> wait(std::latch& lt, const int loop_num)
{
    lt.wait();
    loop_add;
    co_return;
}

static void stl_latch(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        waitgroup_bench<std::latch>(loop_num);
    }
}

CORO_BENCHMARK3(stl_latch, 100, 100000, 100000000);

task<> done(wait_group& wg, const int loop_num)
{
    loop_add;
    wg.done();
    co_return;
}

task<> wait(wait_group& wg, const int loop_num)
{
    co_await wg.wait();
    loop_add;
    co_return;
}

static void coro_waitgroup(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        waitgroup_bench<wait_group>(loop_num);
    }
}

CORO_BENCHMARK3(coro_waitgroup, 100, 100000, 100000000);

BENCHMARK_MAIN();

template<typename waitgroup_type>
void waitgroup_bench(const int loop_num)
{
    const int done_num = thread_num / 2;
    const int wait_num = thread_num - done_num;

    scheduler::init();

    waitgroup_type wg(done_num);

    for (int i = 0; i < done_num; i++)
    {
        submit_to_scheduler(done(wg, loop_num));
    }

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait(wg, loop_num));
    }

    scheduler::start();
    scheduler::loop(false);
}