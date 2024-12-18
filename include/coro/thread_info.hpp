#pragma once
#include <atomic>

namespace coro
{
  class Context;
  struct ThreadInfo
  {
    Context *context{nullptr};
  };

  struct GlobalInfo
  {
    atomic<size_t> context_num;
  };

  inline thread_local ThreadInfo thread_info;
  inline GlobalInfo global_info;
};