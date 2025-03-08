#pragma once

#include <coroutine>

#include "coro/context.hpp"
#include "coro/uring_proxy.hpp"
#include "net/io_info.hpp"

namespace coro::net::detail
{

class base_io_awaiter
{
public:
    base_io_awaiter() noexcept : m_urs(local_context().get_engine().get_free_urs()) {}

    constexpr auto await_ready() noexcept -> bool { return false; }

    auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { m_info.handle = handle; }

    auto await_resume() noexcept -> int32_t { return m_info.result; }

protected:
    io_info             m_info;
    coro::uring::ursptr m_urs;
};

}; // namespace coro::net::detail