#pragma once

#include <atomic>
#include <coroutine>

namespace coro
{
  using std::atomic;
  using std::coroutine_handle;
  using std::memory_order_acquire;
  using std::memory_order_relaxed;
  using std::uintptr_t;

  class Mutex
  {
  public:
    class LockAwaiter
    {
    public:
      explicit LockAwaiter(Mutex &mtx) noexcept : mtx_(mtx) {}

      constexpr bool await_ready() noexcept { return false; }

      bool await_suspend(coroutine_handle<> handle)
      {
        wait_handle_ = handle;
        return register_lock();
      }

      constexpr void await_resume() noexcept {}

    protected:
      bool register_lock() noexcept;

      inline coroutine_handle<> get_wait_handle() noexcept
      {
        return wait_handle_;
      }

      void submit_task() noexcept;

    protected:
      Mutex &mtx_;
      coroutine_handle<> wait_handle_;
    };

  public:
    Mutex() noexcept : state_(nolocked) {}
    ~Mutex() noexcept;

    bool try_lock() noexcept;

    LockAwaiter lock() noexcept { return LockAwaiter(*this); }

  private:
    inline static constexpr uintptr_t nolocked = 0;
    inline static constexpr uintptr_t locked_no_waiting = 1;
    atomic<uintptr_t> state_;
  };

};