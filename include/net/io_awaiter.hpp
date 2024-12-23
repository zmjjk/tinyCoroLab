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
      // io_uring_sqe_set_data(sqe_, &info_);
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
    TcpAcceptAwaiter(int listenfd, int flags) noexcept
    {
      info_.type = Tasktype::TcpAccept;

      // FIXME: this isn't atomic, maybe cause bug?
      io_uring_prep_accept(sqe_, listenfd, nullptr, &len, flags);
      // FIXME: old uring version need set data later
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }

  private:
    inline static socklen_t len = sizeof(sockaddr_in);
  };

  class TcpReadAwaiter : public IoAwaiter
  {
  public:
    TcpReadAwaiter(int sockfd, char *buf, size_t len, int flags) noexcept
    {
      info_.type = Tasktype::TcpRead;

      io_uring_prep_recv(sqe_, sockfd, buf, len, flags);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };

  class TcpWriteAwaiter : public IoAwaiter
  {
  public:
    TcpWriteAwaiter(int sockfd, char *buf, size_t len, int flags) noexcept
    {
      info_.type = Tasktype::TcpWrite;

      io_uring_prep_send(sqe_, sockfd, buf, len, flags);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };

  class TcpCloseAwaiter : public IoAwaiter
  {
  public:
    TcpCloseAwaiter(int sockfd) noexcept
    {
      info_.type = Tasktype::TcpClose;

      io_uring_prep_close(sqe_, sockfd);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };
};
