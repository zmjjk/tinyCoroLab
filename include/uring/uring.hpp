#pragma once

#include <cassert>
#include <functional>
#include <liburing.h>
#include <sys/eventfd.h>
#include "config/config.hpp"

namespace coro
{
#define GETURINGNUM(number) (number & 0xffff)
#define SETTASKNUM (uint64_t(1) << 32)

  using ursptr = io_uring_sqe *;
  using urcptr = io_uring_cqe *;
  using urchandler = std::function<void(urcptr)>;

  class UringProxy
  {
  public:
    void init(unsigned int entry_length) noexcept;

    void deinit() noexcept;

    bool peek_uring() noexcept;

    void wait_uring(int num) noexcept;

    inline void seen_cqe_entry(urcptr cqe) noexcept;

    inline ursptr get_free_sqe() noexcept;

    inline int submit() noexcept;

    size_t handle_for_each_cqe(urchandler f) noexcept;

    uint64_t wait_eventfd() noexcept;

    inline int peek_batch_cqe(urcptr *cqes, unsigned int num) noexcept;

    inline void write_eventfd(uint64_t num) noexcept;

  private:
    [[__attribute_maybe_unused__]] int efd_;
    [[__attribute_maybe_unused__]] io_uring_params para_;
    alignas(config::kCacheLineSize) io_uring ring_;
  };
};
