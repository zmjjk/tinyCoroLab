#pragma once

#include <optional>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net/base_awaiter.hpp"

namespace coro::net
{
  using ::coro::io::IoAwaiter;

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
    explicit TcpCloseAwaiter(int sockfd) noexcept
    {
      info_.type = Tasktype::TcpClose;

      io_uring_prep_close(sqe_, sockfd);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };

  class TcpConnectAwaiter : public IoAwaiter
  {
  public:
    TcpConnectAwaiter(int sockfd, const sockaddr *addr, socklen_t addrlen) noexcept
    {
      info_.type = Tasktype::TcpConnect;
      info_.data = DATACAST(sockfd);

      io_uring_prep_connect(sqe_, sockfd, addr, addrlen);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };

  class StdinAwaiter : public IoAwaiter
  {
  public:
    StdinAwaiter(char *buf, size_t len, int flags) noexcept
    {
      info_.type = Tasktype::Stdin;

      io_uring_prep_read(sqe_, STDIN_FILENO, buf, len, flags);
      io_uring_sqe_set_data(sqe_, &info_);
      local_thread_context()->get_worker().add_wait_task();
    }
  };
};
