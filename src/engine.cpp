#include "coro/engine.hpp"
#include "coro/net/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    linfo.egn = this;
    m_upxy.init(config::kEntryLength);
}

auto engine::deinit() noexcept -> void
{
    m_upxy.deinit();
    m_num_io_wait_submit = 0;
    spsc_queue<coroutine_handle<>> task_queue;
    m_task_queue.swap(task_queue);
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
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<net::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
    int num_task_wait = m_num_io_wait_submit.load(std::memory_order_acquire);
    if (num_task_wait > 0)
    {
        int num = m_upxy.submit();
        num_task_wait -= num;
        assert(num_task_wait == 0);
        m_num_io_running.fetch_add(num, std::memory_order_release); // must set before m_num_io_wait_submit
        m_num_io_wait_submit.fetch_sub(num, std::memory_order_release);
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
    m_num_io_running.fetch_sub(num, std::memory_order_release);
}

auto engine::wake_up() noexcept -> void
{
    m_upxy.write_eventfd(SETTASKNUM);
}
}; // namespace coro::detail
