#pragma once

#include <cassert>
#include <functional>
#include <liburing.h>
#include <sys/eventfd.h>
#include "config/config.hpp"

namespace coro
{
  using ursptr = io_uring_sqe *;
  using urcptr = io_uring_cqe *;
  using urchandler = std::function<void(urcptr)>;

  class UringProxy
  {
  public:
    void init(unsigned int entry_length)
    {
      int res = io_uring_queue_init(entry_length, &ring_, 0);
      assert(res == 0);

      efd_ = eventfd(0, 0);
      assert(efd_ >= 0);
    }

    void deinit()
    {
      io_uring_queue_exit(&ring_);
    }

    bool peek_uring()
    {
      urcptr cqe{nullptr};
      int ret = io_uring_peek_cqe(&ring_, &cqe);
      assert(ret == 0);
      return cqe != nullptr;
    }

    void wait_uring(int num)
    {
      [[__attribute_maybe_unused__]] urcptr cqe;
      int ret;
      if (num == 1) [[likely]]
      {
        ret = io_uring_wait_cqe(&ring_, &cqe);
      }
      else
      {
        ret = io_uring_wait_cqe_nr(&ring_, &cqe, num);
      }
      assert(ret == 0);
    }

    void seen_cqe_entry(urcptr cqe)
    {
      io_uring_cqe_seen(&ring_, cqe);
    }

    ursptr get_free_sqe()
    {
      return io_uring_get_sqe(&ring_);
    }

    int submit()
    {
      return io_uring_submit(&ring_);
    }

    size_t handle_for_each_cqe(urchandler f)
    {
      urcptr cqe;
      unsigned head;
      unsigned i = 0;
      io_uring_for_each_cqe(&ring_, head, cqe)
      {
        f(cqe);
        i++;
      };
      io_uring_cq_advance(&ring_, i);
      return i;
    }

  private:
    [[__attribute_maybe_unused__]] int efd_;
    [[__attribute_maybe_unused__]] io_uring_params para_;
    alignas(config::kCacheLineSize) io_uring ring_;
  };
};
