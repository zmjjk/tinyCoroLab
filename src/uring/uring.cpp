#include "uring/uring.hpp"

namespace coro
{

  void UringProxy::init(unsigned int entry_length) noexcept
  {
    int res = io_uring_queue_init(entry_length, &ring_, 0);
    assert(res == 0);

    efd_ = eventfd(0, 0);
    assert(efd_ >= 0);

    res = io_uring_register_eventfd(&ring_, efd_);
    assert(res == 0);
  }

  void UringProxy::deinit() noexcept
  {
    io_uring_unregister_eventfd(&ring_);
    close(efd_);
    io_uring_queue_exit(&ring_);
  }

  bool UringProxy::peek_uring() noexcept
  {
    urcptr cqe{nullptr};
    int ret = io_uring_peek_cqe(&ring_, &cqe);
    assert(ret == 0);
    return cqe != nullptr;
  }

  void UringProxy::wait_uring(int num) noexcept
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

  void UringProxy::seen_cqe_entry(urcptr cqe) noexcept
  {
    io_uring_cqe_seen(&ring_, cqe);
  }

  ursptr UringProxy::get_free_sqe() noexcept
  {
    return io_uring_get_sqe(&ring_);
  }

  int UringProxy::submit() noexcept
  {
    return io_uring_submit(&ring_);
  }

  size_t UringProxy::handle_for_each_cqe(urchandler f) noexcept
  {
    urcptr cqe;
    unsigned head;
    unsigned i = 0;
    io_uring_for_each_cqe(&ring_, head, cqe)
    {
      f(cqe);
      i++;
    };
    // io_uring_cq_advance(&ring_, i);
    return i;
  }

  int UringProxy::peek_batch_cqe(urcptr *cqes, unsigned int num) noexcept
  {
    return io_uring_peek_batch_cqe(&ring_, cqes, num);
  }

  void UringProxy::write_eventfd(uint64_t num) noexcept
  {
    write(efd_, &num, sizeof(num));
  }

  uint64_t UringProxy::wait_eventfd() noexcept
  {
    uint64_t u;
    read(efd_, &u, sizeof(u));
    return u;
  }
};