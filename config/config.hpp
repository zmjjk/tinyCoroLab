#pragma once

#include <cstddef>
#include <cstdint>

namespace coro::config
{
  // ========================== uring configuration ===========================
  constexpr unsigned int kEntryLength = 1024;

  // =========================== cpu configuration ============================
  constexpr size_t kCacheLineSize = 64;

  // ========================= context configuration ==========================
  using ctx_id = uint32_t;
  constexpr size_t kQueCap = 1024;
};