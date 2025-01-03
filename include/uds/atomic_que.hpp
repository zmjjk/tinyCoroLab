#include "atomic_queue/atomic_queue.h"
#include "config.hpp"
#include "uds/ring_cursor.hpp"

namespace coro::ds
{
  // lib AtomicQueue
  template <class Queue, size_t Capacity>
  struct CapacityToConstructor : Queue
  {
    CapacityToConstructor()
        : Queue(Capacity) {}
  };

  template <typename T>
  using AtomicQueue = CapacityToConstructor<atomic_queue::AtomicQueueB2<T, std::allocator<T>,
                                                                        false, false, true>,
                                            config::kQueCap>;

  using std::array;
  // lib my impl
  template <typename T>
  class RingQueue
  {
  public:
    bool was_empty() noexcept
    {
      return rcur_.isEmpty();
    }

    size_t was_size() noexcept
    {
      return rcur_.size();
    }

    void push(T &elem) noexcept
    {
      rbuf_[rcur_.tail()] = elem;
      rcur_.push();
    }

    T pop() noexcept
    {
      assert(!rcur_.isEmpty());
      auto pos = rcur_.head();
      rcur_.pop();
      return rbuf_[pos];
    }

  private:
    RingCursor<size_t, config::kQueCap> rcur_;
    array<T, config::kQueCap> rbuf_;
  };
};