#include "atomic_queue/atomic_queue.h"
#include "config.hpp"

namespace coro::ds
{
  template <class Queue, size_t Capacity>
  struct CapacityToConstructor : Queue
  {
    CapacityToConstructor()
        : Queue(Capacity) {}
  };

  template <typename T>
  using SpscQueue = CapacityToConstructor<atomic_queue::AtomicQueueB2<T, std::allocator<T>,
                                                                      false, false, true>,
                                          config::kQueCap>;
};