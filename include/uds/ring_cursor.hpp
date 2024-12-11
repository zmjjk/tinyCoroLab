#pragma once

#include <bit>
#include <concepts>

namespace coro::ds
{
  /**
   * @brief
   *
   * @tparam T
   * @tparam capacity
   * @tparam capacity,
   * typename
   */
  template <std::unsigned_integral T, T capacity,
            typename = std::enable_if<std::has_single_bit(capacity) && (capacity > 1)>::type>
  class RingCursor
  {
  public:
    RingBuf() noexcept {}
    ~RingBuf() noexcept {}

  public:
    inline T head() const noexcept
    {
      return head_ & kMask;
    }

    inline T tail() const noexcept
    {
      return tail_ & kMask;
    }

    inline T isEmpty() const noexcept
    {
      return head_ == tail_;
    }

    inline T size() const noexcept
    {
      return tail_ - head_;
    }

    inline T leftSize() const noexcept
    {
      return capacity - (tail_ - head_);
    }

    inline T hasSpace() const noexcept
    {
      return bool(leftSize());
    }

    inline void push() noexcept
    {
      tail_ += 1;
    }

    inline void pop() noexcept
    {
      head_ += 1;
    }

    // TODO: thread-safe && lock-free api impl

  private:
    static constexpr T kMask = capacity - 1;

    T head_{0};
    T tail_{0};
  };

};