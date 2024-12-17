#pragma once

#include <bit>
#include <concepts>
#include <mutex>

#include "utils/atom.hpp"

namespace coro::ds
{
  using std::lock_guard;
  using std::mutex;

  // TODO: remove mutex
  /**
   * @brief lock free ring cursor
   *
   * @tparam T
   * @tparam capacity
   * @tparam capacity,
   * typename
   */
  template <std::unsigned_integral T, T capacity,
            bool thread_safe = true,
            typename = std::enable_if<std::has_single_bit(capacity) && (capacity > 1)>::type>
  class RingCursor
  {
  public:
    RingBuf() noexcept {}
    ~RingBuf() noexcept {}

  public:
    inline T head() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      // if constexpr (thread_safe)
      // {
      //   return (cast_atomic(head_).load()) & kMask;
      // }
      // else
      // {
      //   return head_ & kMask;
      // }
      return head_ & kMask;
    }

    inline T tail() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      // if constexpr (thread_safe)
      // {
      //   return (cast_atomic(tail_).load()) & kMask;
      // }
      // else
      // {
      //   return tail_ & kMask;
      // }
      return tail_ & kMask;
    }

    inline T isEmpty() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      return head_ == tail_;
    }

    inline T size() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      return tail_ - head_;
    }

    inline T leftSize() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      return capacity - (tail_ - head_);
    }

    inline T hasSpace() const noexcept
    {
      lock_guard<mutex> lk(mtx_);
      return bool(leftSize());
    }

    inline void push() noexcept
    {
      lock_guard<mutex> lk(mtx_);
      tail_ += 1;
    }

    inline void pop() noexcept
    {
      lock_guard<mutex> lk(mtx_);
      head_ += 1;
    }

  private:
    static constexpr T kMask = capacity - 1;

    T head_{0};
    T tail_{0};
    mutex mtx_;
  };

};