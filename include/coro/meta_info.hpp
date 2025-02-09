#pragma once

#include <atomic>

#include "coro/attribute.hpp"

namespace coro
{
class context;
};

namespace coro::detail
{
using config::ctx_id;
using std::atomic;

struct CORO_ALIGN local_info
{
    context* ctx{nullptr};
    // TODO: Add more local info
};

struct global_info
{
    atomic<ctx_id> context_id;
};

inline thread_local local_info linfo;
inline global_info             ginfo;
}; // namespace coro::detail