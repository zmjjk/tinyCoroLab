#include "coro/engine.hpp"
#include "coro/log.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    m_upxy.init(config::kEntryLength);
    m_num_io_wait_submit = 0;
    m_num_io_running     = 0;

    spsc_queue<coroutine_handle<>> task_queue;
    m_task_queue.swap(task_queue);
}

auto engine::deinit() noexcept -> void
{
    m_upxy.deinit();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    auto coro = m_task_queue.pop();
    assert(bool(coro));
    return coro;
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    assert(handle != nullptr && "engine get nullptr task handle");
    m_task_queue.push(handle);
    wake_up();
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    // FIXME: clean task handle
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
}

auto engine::poll_submit() noexcept -> void
{
    int num_task_wait = m_num_io_wait_submit.load(memory_order_relaxed);
    if (num_task_wait > 0)
    {
        int num = m_upxy.submit();
        num_task_wait -= num;
        assert(num_task_wait == 0);
        m_num_io_wait_submit.fetch_sub(num, memory_order_relaxed);
    }

    auto cnt = m_upxy.wait_eventfd();

    auto uringnum = GETURINGNUM(cnt);

    auto num = m_upxy.peek_batch_cqe(m_urc.data(), uringnum);

    assert(num == uringnum && "unexpected uringnum by peek cqe");
    for (int i = 0; i < num; i++)
    {
        handle_cqe_entry(m_urc[i]);
    }
    m_upxy.cq_advance(num);
}

auto engine::wake_up() noexcept -> void
{
    m_upxy.write_eventfd(SETTASKNUM);
}
}; // namespace coro::detail
