#include <algorithm>
#include <latch>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename latch_type>
void latch_bench(const int loop_num);

task<> countdown(std::latch& lt, const int loop_num)
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
        latch_bench<std::latch>(loop_num);
    }
}

BENCHMARK(stl_latch)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(100000)
    ->Arg(100000000);

task<> countdown(latch& lt, const int loop_num)
{
    loop_add;
    lt.count_down();
    co_return;
}

task<> wait(latch& lt, const int loop_num)
{
    co_await lt.wait();
    loop_add;
    co_return;
}

static void coro_latch(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        latch_bench<coro::latch>(loop_num);
    }
}

BENCHMARK(coro_latch)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(100000)
    ->Arg(100000000);

BENCHMARK_MAIN();

template<typename latch_type>
void latch_bench(const int loop_num)
{
    const int countdown_num = thread_num / 2;
    const int wait_num      = thread_num - countdown_num;

    scheduler::init();

    latch_type lt(countdown_num);

    for (int i = 0; i < countdown_num; i++)
    {
        submit_to_scheduler(countdown(lt, loop_num));
    }

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait(lt, loop_num));
    }

    scheduler::start();
    scheduler::loop(false);
}