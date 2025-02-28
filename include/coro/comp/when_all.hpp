#pragma once

#include <array>
#include <tuple>

#include "coro/attribute.hpp"
#include "coro/comp/latch.hpp"
#include "coro/concepts/awaitable.hpp"

namespace coro
{
namespace detail
{

template<typename T>
class when_all_ready_awaitable;

template<>
class when_all_ready_awaitable<std::tuple<>>
{
public:
    constexpr when_all_ready_awaitable() noexcept {}
    explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}

    constexpr auto await_ready() const noexcept -> bool { return true; }
    constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> bool { return false; }
    constexpr auto await_resume() const noexcept -> std::tuple<> { return {}; }
};

template<typename... task_types>
class when_all_ready_awaitable<std::tuple<task_types...>>
{
public:
    explicit when_all_ready_awaitable(task_types&&... tasks) noexcept
        : m_latch(sizeof...(task_types)),
          m_tasks(std::move(tasks)...)
    {
    }

    explicit when_all_ready_awaitable(std::tuple<task_types...>&& tasks) noexcept
        : m_latch(sizeof...(task_types)),
          m_tasks(std::move(tasks))
    {
    }

    CORO_NO_COPY_MOVE(when_all_ready_awaitable);

    auto operator co_await() noexcept
    {
        std::apply([this](auto&&... tasks) { ((tasks.start(m_latch)), ...); }, m_tasks);
        return m_latch.wait();
    }

private:
    latch                     m_latch;
    std::tuple<task_types...> m_tasks;
};

template<typename T>
class when_all_task_promise;

template<concepts::void_no_ref_pod T>
class when_all_task;

template<concepts::void_no_ref_pod return_type>
class when_all_task_promise_base
{
public:
    using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<return_type>>;

    when_all_task_promise_base() noexcept {}

    auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    auto final_suspend() noexcept
    {
        struct completion_notifier
        {
            auto await_ready() const noexcept -> bool { return false; }
            auto await_suspend(coroutine_handle_type coroutine) const noexcept -> void
            {
                coroutine.promise().m_latch->count_down();
            }
            auto await_resume() const noexcept {}
        };

        return completion_notifier{};
    }

    auto unhandled_exception() noexcept
    {
        // Keep simple, just ignore exception
    }

    auto start(latch& l) noexcept { m_latch = &l; }

protected:
    latch* m_latch{nullptr};
};

template<concepts::no_ref_pod return_type>
class when_all_task_promise<return_type> : public when_all_task_promise_base<return_type>
{
public:
    using storage_type = std::add_pointer_t<return_type>;

    auto get_return_object() noexcept -> decltype(auto);

    auto return_value(return_type value) noexcept -> void { *m_data = value; }

    auto set_pointer(storage_type data_ptr) noexcept -> void { m_data = data_ptr; }

private:
    storage_type m_data;
};

template<>
class when_all_task_promise<void> : public when_all_task_promise_base<void>
{
public:
    auto get_return_object() noexcept -> decltype(auto);

    constexpr auto return_void() noexcept -> void {}
};

template<concepts::void_no_ref_pod return_type>
class when_all_task
{
public:
    using promise_type          = when_all_task_promise<return_type>;
    using coroutine_handle_type = typename promise_type::coroutine_handle_type;

    explicit when_all_task(coroutine_handle_type handle) noexcept : m_handle(handle) {}
    when_all_task(const when_all_task&) = delete;
    when_all_task(when_all_task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}

    when_all_task& operator=(const when_all_task&) = delete;
    when_all_task& operator=(when_all_task&&)      = delete;

    ~when_all_task() noexcept
    {
        if (m_handle != nullptr)
        {
            m_handle.destroy();
        }
    }

    void start(latch& l) noexcept
    {
        m_handle.promise().start(l);
        local_context().submit_task(m_handle);
    }

private:
    coroutine_handle_type m_handle;
};

template<
    concepts::awaitable awaitable,
    typename return_type = typename concepts::awaitable_traits<awaitable&&>::awaiter_return_type>
[[CORO_AWAIT_HINT]] static auto make_when_all_task(awaitable&& a) -> when_all_task<return_type>
{
    if constexpr (std::is_void_v<return_type>)
    {
        co_await static_cast<awaitable&&>(a);
        co_return;
    }
    else
    {
        co_return co_await static_cast<awaitable&&>(a);
    }
}

}; // namespace detail

template<concepts::awaitable... awaitables_type>
[[CORO_AWAIT_HINT]] static auto when_all(awaitables_type... awaitables) noexcept -> decltype(auto)
{
    return detail::when_all_ready_awaitable<std::tuple<
        detail::when_all_task<typename concepts::awaitable_traits<awaitables_type>::awaiter_return_type>...>>(
        std::make_tuple(detail::make_when_all_task(std::move(awaitables))...));
}

namespace detail
{
template<concepts::no_ref_pod return_type>
auto when_all_task_promise<return_type>::get_return_object() noexcept -> decltype(auto)
{
    return when_all_task<return_type>{
        when_all_task_promise_base<return_type>::coroutine_handle_type::from_promise(*this)};
}

auto when_all_task_promise<void>::get_return_object() noexcept -> decltype(auto)
{
    return when_all_task<void>{when_all_task_promise_base<void>::coroutine_handle_type::from_promise(*this)};
}

}; // namespace detail

}; // namespace coro
