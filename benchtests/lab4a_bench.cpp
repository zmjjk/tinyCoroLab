#include <algorithm>
#include <future>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename event_type>
void event_bench(const int loop_num);

task<> set(std::promise<void>& promise, const int loop_num)
{
    loop_add;
    promise.set_value();
    co_return;
}

task<> wait(std::future<void>& future, const int loop_num)
{
    future.wait();
    loop_add;
    co_return;
}

static void stl_future(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        event_bench<std::promise<void>>(loop_num);
    }
}

BENCHMARK(stl_future)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(100000)
    ->Arg(100000000);

task<> set(event<>& ev, const int loop_num)
{
    loop_add;
    ev.set();
    co_return;
}

task<> wait(event<>& ev, const int loop_num)
{
    co_await ev.wait();
    loop_add;
}

static void coro_event(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        event_bench<event<>>(loop_num);
    }
}

BENCHMARK(coro_event)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(100000)
    ->Arg(100000000);

BENCHMARK_MAIN();

template<typename event_type>
void event_bench(const int loop_num)
{
    scheduler::init();

    if constexpr (std::is_same_v<event_type, std::promise<void>>)
    {
        std::promise<void> pro;
        auto               fut = pro.get_future();

        for (int i = 0; i < thread_num - 1; i++)
        {
            submit_to_scheduler(wait(fut, loop_num));
        }
        submit_to_scheduler(set(pro, loop_num));

        scheduler::start();
        scheduler::loop(false);
    }
    else
    {
        event_type ev;

        for (int i = 0; i < thread_num - 1; i++)
        {
            submit_to_scheduler(wait(ev, loop_num));
        }
        submit_to_scheduler(set(ev, loop_num));

        scheduler::start();
        scheduler::loop(false);
    }
}