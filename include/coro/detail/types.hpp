#pragma once

#include <cstdint>

namespace coro::detail
{

// TODO: Add lifo and fifo strategy support
enum class schedule_strategy : uint8_t
{
    FIFO, // default
    LIFO,
    None
};

// TODO: Add awaiter base support
using awaiter_ptr = void*;

}; // namespace coro::detail