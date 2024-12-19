#pragma once

#include <concepts>
#include <iostream>

namespace coro
{
  // FIXME: why complie error?
  using std::same_as;
  template <typename T>
  concept unlockype = requires(T mtx) {
    { mtx.unlock() } -> same_as<void>;
  };

  template <typename T>
  class LockGuard
  {
  public:
    explicit LockGuard(T &mtx) noexcept : mtx_(mtx) {}
    ~LockGuard() noexcept
    {
      mtx_.unlock();
    }

    LockGuard(LockGuard &&other) noexcept : mtx_(other.mtx_) {}

    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;
    LockGuard &operator=(LockGuard &&) = delete;

  private:
    T &mtx_;
  };
};