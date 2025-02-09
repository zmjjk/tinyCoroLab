#pragma once

#include "config.h"

// This is a GCC extension; define it only for GCC and compilers that emulate GCC.
#if defined(__GNUC__) && !defined(__clang__)
    #define __ATTRIBUTE__(attr) __attribute__((attr))
#else
    #define __ATTRIBUTE__(attr)
#endif

#define CORO_AWAIT_HINT   nodiscard("Did you forget to co_await?")
#define CORO_DISCARD_HINT nodiscard("Discard is improper")
#define CORO_ALIGN        alignas(::coro::config::kCacheLineSize)
#define CORO_TEST_USED