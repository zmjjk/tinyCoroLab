#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

namespace coro::utils
{
void set_fd_noblock(int fd) noexcept;

inline void sleep(int64_t t) noexcept
{
    std::this_thread::sleep_for(std::chrono::seconds(t));
}
}; // namespace coro::utils