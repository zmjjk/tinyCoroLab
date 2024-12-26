#pragma once

#include <bit>
#include <concepts>

#ifdef USEMUTEX
#include <mutex>
#else
#include <atomic>
#endif

#include "config.hpp"

namespace coro::ds
{
#ifdef USEMUTEX
      using std::lock_guard;
      using std::mutex;
#else
      using std::atomic;
#endif
      using std::memory_order_relaxed;

      // FIXME: please test the correctness
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
      class [[deprecated("Use SpscQueue instead")]] RingCursor
      {
      public:
            RingCursor() noexcept {}
            ~RingCursor() noexcept {}

      public:
            inline T head() const noexcept
            {
#ifdef USEMUTEX
                  lock_guard<mutex> lk(mtx_);
                  return head_ & kMask;
#else
                  return head_.load(memory_order_relaxed) & kMask;
#endif
            }

            inline T tail() const noexcept
            {
#ifdef USEMUTEX
                  lock_guard<mutex> lk(mtx_);
                  return tail_ & kMask;
#else
                  return tail_.load(memory_order_relaxed) & kMask;
#endif
            }

            inline T size() const noexcept
            {
#ifdef USEMUTEX
                  lock_guard<mutex> lk(mtx_);
                  return tail_ - head_;
#else
                  auto tail = tail_.load(memory_order_relaxed);
                  auto head = head_.load(memory_order_relaxed);
                  return tail_ - head_;
#endif
            }

            inline T isEmpty() const noexcept
            {
                  return size() == 0;
            }

            // inline T leftSize() const noexcept
            // {
            //   lock_guard<mutex> lk(mtx_);
            //   return capacity - (tail_ - head_);
            // }

            // inline T hasSpace() const noexcept
            // {
            //   lock_guard<mutex> lk(mtx_);
            //   return bool(leftSize());
            // }

            inline void push() noexcept
            {
#ifdef USEMUTEX
                  tail_++;
#else
                  tail_.fetch_add(1, memory_order_relaxed);
#endif
            }

            inline void pop() noexcept
            {
#ifdef USEMUTEX
                  head_++;
#else
                  head_.fetch_add(1, memory_order_relaxed);
#endif
            }

      private:
            static constexpr T kMask = capacity - 1;

#ifdef USEMUTEX
            mutable mutex mtx_;
            alignas(config::kCacheLineSize) T head_{0};
            alignas(config::kCacheLineSize) T tail_{0};
#else
            alignas(config::kCacheLineSize) atomic<T> head_{0};
            alignas(config::kCacheLineSize) atomic<T> tail_{0};
#endif
      };

};