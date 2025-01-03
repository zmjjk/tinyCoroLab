#pragma once

#include <functional>

#include "lock/mutex.hpp"
#include "lock/spin_lock.hpp"

namespace coro
{
  using std::function;
  using CondType = function<bool()>;

  class CondVar
  {
  public:
    class CvAwaiter : public Mutex::LockAwaiter
    {
      friend CondVar;

    public:
      CvAwaiter(Mutex &mtx, Context &ctx, CondVar &cv) noexcept
          : LockAwaiter(mtx, ctx), cv_(cv) {}
      CvAwaiter(Mutex &mtx, Context &ctx, CondVar &cv, CondType &cond) noexcept
          : LockAwaiter(mtx, ctx), cv_(cv), cond_(cond) {}

    protected:
      bool register_lock() noexcept;

      void register_cv() noexcept;

      void wake_up() noexcept;

      void submit_task() noexcept override;

    private:
      CondType cond_;
      CondVar &cv_;
    };

  public:
    CvAwaiter wait(Mutex &mtx) noexcept
    {
      return CvAwaiter(mtx, *(thread_info.context), *this);
    }

    CvAwaiter wait(Mutex &mtx, CondType &cond) noexcept
    {
      return CvAwaiter(mtx, *(thread_info.context), *this, cond);
    }

    void notify_one() noexcept;

    void notify_all() noexcept;

  private:
    // Spinlock lock_; // maybe unused
    CvAwaiter *head_{nullptr};
    CvAwaiter *tail_{nullptr};
  };
};
