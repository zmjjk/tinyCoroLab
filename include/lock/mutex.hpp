#pragma once

#include <atomic>
#include <coroutine>
#include <iostream>

#include "lock/lock_guard.hpp"

namespace coro
{
  using std::atomic;
  using std::coroutine_handle;
  using std::memory_order_acquire;
  using std::memory_order_relaxed;
  using std::memory_order_release;
  using std::uintptr_t;

  class Mutex
  {
  public:
    class LockAwaiter
    {
      friend Mutex;

    public:
      explicit LockAwaiter(Mutex &mtx) noexcept : mtx_(mtx) {}

      constexpr bool await_ready() noexcept
      {
        return false;
      }

      bool await_suspend(coroutine_handle<> handle) noexcept
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
      LockAwaiter *next_{nullptr};
    };

    class LockGuardAwaiter : public LockAwaiter
    {
    public:
      using LockGuardType = LockGuard<Mutex>;

      LockGuardAwaiter(Mutex &mtx) noexcept : LockAwaiter(mtx) {}

    public:
      LockGuardType await_resume() noexcept
      {
        return LockGuardType(mtx_);
      }
    };

  public:
    Mutex() noexcept : state_(nolocked) {}
    ~Mutex() noexcept;

    bool try_lock() noexcept;

    LockAwaiter lock() noexcept { return LockAwaiter(*this); }

    void unlock() noexcept;

    LockGuardAwaiter lock_guard() noexcept
    {
      return LockGuardAwaiter(*this);
    }

  private:
    inline static constexpr uintptr_t nolocked = 1;
    inline static constexpr uintptr_t locked_no_waiting = 0; // nullptr
    atomic<uintptr_t> state_;
  };

};