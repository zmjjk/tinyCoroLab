#include "coro/context.hpp"
#include "config.h"
#include "coro/meta_info.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.init();
    linfo.ctx = this;
    
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.deinit();
    linfo.ctx = nullptr;
    
}

auto context::start() noexcept -> void
{
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            this->run(token);
            this->deinit();
        });
    //m_job_ready.store(true, std::memory_order_release);
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
   // 关键：等待标志变为 true，使用 memory_order_acquire 确保能看到 release 之前的写入
/*    while (!m_job_ready.load(std::memory_order_acquire)) {
    // 可以考虑短暂休眠或让出 CPU，防止忙等待
    std::this_thread::yield();
    }
    // 此时可以安全地假设 m_job 非空（基于 start 的逻辑）
    // 但为了应对极端情况或析构时的并发调用，保留 if 仍然是好习惯
    if (m_job) { // 保留 if 仍然更健壮
    m_engine.wake_up(1);
    m_job->request_stop();
    } */
     
        m_engine.wake_up(1);
        m_job->request_stop();
     
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    // 原子地增加等待计数
    if (register_cnt > 0) {
        m_wait_cnt.fetch_add(register_cnt, std::memory_order_relaxed);
    }
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    // 原子地减少等待计数
    if (register_cnt > 0) {
        m_wait_cnt.fetch_sub(register_cnt, std::memory_order_relaxed);
    }
}

auto context::run(stop_token token) noexcept -> void
{
    while (true)
    {
        process_work();
        if (token.stop_requested() && empty_wait_task())
        {
            if (!m_engine.ready())
            {
                break;
            }
            else // 此时任务队列还有任务，优先执行任务队列里的任务
            {
                continue;
            }
        }
        poll_work();
        if (token.stop_requested() && empty_wait_task() && !m_engine.ready())
        {
            break;
        }
    }
}
// 驱动engine从任务队列取出任务并执行
auto context::process_work() noexcept -> void
{
    auto num = m_engine.num_task_schedule();
    for (int i = 0; i < num; i++)
    {
        m_engine.exec_one_task();
    }
}
// 驱动engine执行IO任务
auto context::poll_work() noexcept -> void { m_engine.poll_submit(); }
// 判断是否没有IO任务以及引用计数是否为0
auto context::empty_wait_task() noexcept -> bool
{
    return m_wait_cnt.load(memory_order_acquire) == 0 && m_engine.empty_io();
}

}; // namespace coro