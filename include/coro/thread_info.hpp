#pragma once

namespace coro
{
  class Context;
  struct ThreadInfo
  {
    Context *context{nullptr};
  };

  inline thread_local ThreadInfo thread_info;
};