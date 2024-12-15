#include <cassert>

#include "lock/mutex.hpp"
#include "coro/context.hpp"

namespace coro
{
  bool Mutex::LockAwaiter::register_lock()
  {
    while (true)
    {
      auto state = mtx_.state_.load(memory_order_acquire);
      if (state == Mutex::nolocked)
      {
        if (mtx_.state_.compare_exchange_strong(
                state, Mutex::locked_no_waiting, memory_order_acquire, memory_order_relaxed))
        {
          return false;
        }
      }
      else
      {
      }
    }
  }

  void Mutex::LockAwaiter::submit_task()
  {
    local_thread_context()->submit_task(wait_handle_);
  }

  Mutex::~Mutex()
  {
    [[__attribute_maybe_unused__]] auto state = state_.load(memory_order_relaxed);
    assert(state == nolocked);
  }

  bool Mutex::try_lock()
  {
    auto target = nolocked;
    return state_.compare_exchange_strong(target, locked_no_waiting,
                                          memory_order_acquire, memory_order_acquire);
  }

};