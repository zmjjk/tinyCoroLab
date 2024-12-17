#pragma once

#include <atomic>
#include <type_traits>

namespace coro
{
  using std::addressof;
  using std::atomic;

  // TODO: add AtomicSupport
  // template <typename T>
  // concept AtomicSupport = requires(T v) {
  //   atomic<T> v;
  // };

  template <typename T>
  inline atomic<T> &cast_atomic(T &value) noexcept
  {
    return *reinterpret_cast<atomic<T> *>(addressof(value));
  }
};