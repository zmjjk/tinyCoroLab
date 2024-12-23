#pragma once

#include <cstring>
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "config.hpp"
#include "net/io_awaiter.hpp"

namespace coro::net
{
  class TcpAcceptor
  {
  public:
    TcpAcceptor(int sockfd) noexcept : sockfd_(sockfd) {}

    TcpReadAwaiter read(char *buf, size_t len, int flags = 0) noexcept
    {
      return TcpReadAwaiter(sockfd_, buf, len, flags);
    }

    TcpWriteAwaiter write(char *buf, size_t len, int flags = 0) noexcept
    {
      return TcpWriteAwaiter(sockfd_, buf, len, flags);
    }

  private:
    int sockfd_;
  };

  class TcpListener
  {
  public:
    TcpListener(int port = ::coro::config::kDefaultPort) noexcept;

    TcpAcceptAwaiter accept(int flags = 0) noexcept;

  private:
    int listenfd_;
    int port_;
    sockaddr_in servaddr_;
  };

};