#include <algorithm>
#include <mutex>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename mutex_type>
void mutex_bench(const int loop_num);

static task<> add(std::mutex& mtx, size_t& cnt, const int loop_num)
{
    mtx.lock();
    loop_add;
    mtx.unlock();
    co_return;
}

static void stl_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        // const int loop_num = state.range(0);

        // thread_pool pool;
        // std::mutex  mtx;
        // size_t      cnt = 0;
        // for (int i = 0; i < thread_num; i++)
        // {
        //     pool.submit_task([&]() { add(mtx, cnt, loop_num); });
        // }

        // pool.start();
        // pool.join();
        const int loop_num = state.range(0);
        mutex_bench<std::mutex>(loop_num);
    }
}

BENCHMARK(stl_mutex)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(10000)
    ->Arg(1000000);

static task<> add(detail::spinlock& mtx, size_t& cnt, const int loop_num)
{
    mtx.lock();
    loop_add;
    mtx.unlock();
    co_return;
}

static void spin_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        // const int loop_num = state.range(0);

        // thread_pool      pool;
        // detail::spinlock mtx;
        // size_t           cnt = 0;
        // for (int i = 0; i < thread_num; i++)
        // {
        //     pool.submit_task([&]() { add(mtx, cnt, loop_num); });
        // }

        // pool.start();
        // pool.join();
        const int loop_num = state.range(0);
        mutex_bench<detail::spinlock>(loop_num);
    }
}

BENCHMARK(spin_mutex)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(10000)
    ->Arg(1000000);

static task<> add(mutex& mtx, size_t& cnt, const int loop_num)
{
    co_await mtx.lock();
    loop_add;
    mtx.unlock();
}

static void coro_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        mutex_bench<mutex>(loop_num);
    }
}

BENCHMARK(coro_mutex)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Arg(100)
    ->Arg(10000)
    ->Arg(1000000);

BENCHMARK_MAIN();

template<typename mutex_type>
void mutex_bench(const int loop_num)
{
    scheduler::init();

    mutex_type mtx;
    size_t     cnt = 0;

    for (int i = 0; i < thread_num; i++)
    {
        submit_to_scheduler(add(mtx, cnt, loop_num));
    }

    scheduler::start();
    scheduler::loop(false);
}
