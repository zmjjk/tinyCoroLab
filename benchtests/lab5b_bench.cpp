#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

/*************************************************************
 *                        notifyall                          *
 *************************************************************/

template<typename cv_type, typename mtx_type>
void cv_notifyall_bench(const int loop_num);

task<> notify_all(std::condition_variable& cv, std::mutex& mtx, int& gid, const int id, const int loop_num)
{
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&]() -> bool { return id == gid; });
    loop_add;
    gid += 1;
    cv.notify_all();
    mtx.unlock();
    co_return;
}

static void stl_cv_notifyall(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        cv_notifyall_bench<std::condition_variable, std::mutex>(loop_num);
    }
}

CORO_BENCHMARK3(stl_cv_notifyall, 100, 100000, 100000000);

task<> notify_all(condition_variable& cv, mutex& mtx, int& gid, const int id, const int loop_num)
{
    co_await mtx.lock_guard();
    cv.wait(mtx, [&]() { return gid == id; });
    cv.notify_all();
}

static void coro_cv_notifyall(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        cv_notifyall_bench<condition_variable, mutex>(loop_num);
    }
}

CORO_BENCHMARK3(coro_cv_notifyall, 100, 100000, 100000000);

/*************************************************************
 *                        notifyone                          *
 *************************************************************/
const int notifyone_run_cnt = 1000;

template<typename cv_type, typename mtx_type>
void cv_notifyone_bench();

task<> notify_one(std::condition_variable& cv, std::mutex& mtx, int& gid, const int id, int run_cnt)
{
    while (run_cnt > 0)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&]() -> bool { return id == gid; });
        gid = (gid + 1) % 2;
        run_cnt -= 1;
        cv.notify_one();
    }
    co_return;
}

static void stl_cv_notifyone(benchmark::State& state)
{
    for (auto _ : state)
    {
        cv_notifyone_bench<std::condition_variable, std::mutex>();
    }
}

CORO_BENCHMARK(stl_cv_notifyone);

task<> notify_one(condition_variable& cv, mutex& mtx, int& gid, const int id, int run_cnt)
{
    while (run_cnt > 0)
    {
        co_await mtx.lock_guard();
        cv.wait(mtx, [&]() { return id == gid; });
        gid = (gid + 1) % 2;
        run_cnt -= 1;
        cv.notify_one();
    }
}

static void coro_cv_notifyone(benchmark::State& state)
{
    for (auto _ : state)
    {
        cv_notifyone_bench<condition_variable, mutex>();
    }
}

CORO_BENCHMARK(coro_cv_notifyone);

BENCHMARK_MAIN();

template<typename cv_type, typename mtx_type>
void cv_notifyall_bench(const int loop_num)
{
    scheduler::init();

    cv_type  cv;
    mtx_type mtx;

    int gid = 0;

    for (int i = thread_num - 1; i >= 0; i--)
    {
        submit_to_scheduler(notify_all(cv, mtx, gid, i, loop_num));
    }

    scheduler::start();
    scheduler::loop(false);
}

template<typename cv_type, typename mtx_type>
void cv_notifyone_bench()
{
    scheduler::init();

    cv_type  cv;
    mtx_type mtx;

    int gid = 0;

    for (int i = 0; i < 2; i++)
    {
        submit_to_scheduler(notify_one(cv, mtx, gid, i, notifyone_run_cnt));
    }

    scheduler::start();
    scheduler::loop(false);
}