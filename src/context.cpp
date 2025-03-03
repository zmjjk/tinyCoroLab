#include "coro/context.hpp"
#include "coro/log.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
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
}

auto context::stop() noexcept -> void
{
    m_job->request_stop();
    m_engine.wake_up();
    m_job->join();
}

auto context::init() noexcept -> void
{
    linfo.ctx = this;
    m_engine.init();
}

auto context::deinit() noexcept -> void
{
    linfo.ctx = nullptr;
    m_engine.deinit();
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
            else
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

auto context::process_work() noexcept -> void
{
    auto num = m_engine.num_task_schedule();
    for (int i = 0; i < num; i++)
    {
        m_engine.exec_one_task();
    }
}

}; // namespace coro