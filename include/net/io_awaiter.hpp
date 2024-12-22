#pragma once

#include <optional>
#include <coroutine>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>

#include "net/io_info.hpp"
#include "uring/uring.hpp"

#include "coro/context.hpp"

namespace coro::net
{
  using std::coroutine_handle;

  class IoAwaiter
  {
  public:
    IoAwaiter() noexcept : sqe_(local_thread_context()->get_worker().get_free_urs())
    {
      std::cout << "ioawaiter construct begin " << (sqe_ == nullptr) << std::endl;
      io_uring_sqe_set_data(sqe_, &info_);
      std::cout << "ioawaiter construct finish" << std::endl;
    }

    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
      info_.handle = handle;
    }

    int32_t await_resume() noexcept { return info_.result; }

  protected:
    IoInfo info_;
    ursptr sqe_;
  };

  class TcpAcceptAwaiter : public IoAwaiter
  {
  public:
    TcpAcceptAwaiter(int listenfd)
    {
      info_.type = Tasktype::Accept;

      // FIXME: this isn't atomic, maybe cause bug?
      io_uring_prep_accept(sqe_, listenfd, nullptr, &len, 0);
      local_thread_context()->get_worker().add_wait_task();
    }

    // FIXME: remove
    ~TcpAcceptAwaiter() noexcept
    {
      std::cout << "ioawaiter destruct" << std::endl;
    }

  private:
    inline static socklen_t len = sizeof(sockaddr_in);
  };

};
