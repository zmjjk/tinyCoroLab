#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace coro::config
{
  // ========================== log configuration =============================
  // #define TRACE 0
  // #define DEBUG 1
  // #define INFO 2
  // #define WARN 3
  // #define ERR 4
  // #define CRITICAL 5
  // #define OFF 6

#define LOG_LEVEL info

  constexpr const char *kLogFileName = "logs/coro.log";
  constexpr int64_t kFlushDura = 3;

  // ========================== uring configuration ===========================
  constexpr unsigned int kEntryLength = 1024;

  // =========================== cpu configuration ============================
  constexpr size_t kCacheLineSize = 64;

  // ========================= context configuration ==========================
  using ctx_id = uint32_t;
  constexpr size_t kQueCap = 1024;
};