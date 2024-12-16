#pragma once

#include <concepts>

namespace coro
{
  using std::same_as;
  template <typename T>
  concept unlockype = requires(T mutex) {
    { mutex.unlock() } -> same_as<void>;
  };

  template <unlockype T>
  class LockGuard
  {
  public:
    explicit LockGuard(T &mtx) noexcept : mtx_(mtx) {}
    ~LockGuard() { mtx_.unlock(); }

    LockGuard(const LockGuard &) = delete;
    LockGuard(LockGuard &&) = delete;
    LockGuard &operator=(const LockGuard &) = delete;
    LockGuard &operator=(LockGuard &&) = delete;

  private:
    T &mtx_;
  };
};