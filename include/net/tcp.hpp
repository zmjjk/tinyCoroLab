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
  class TcpConnector
  {
  public:
    explicit TcpConnector(int sockfd) noexcept : sockfd_(sockfd) {}

    TcpReadAwaiter read(char *buf, size_t len, int flags = 0) noexcept
    {
      return TcpReadAwaiter(sockfd_, buf, len, flags);
    }

    TcpWriteAwaiter write(char *buf, size_t len, int flags = 0) noexcept
    {
      return TcpWriteAwaiter(sockfd_, buf, len, flags);
    }

    TcpCloseAwaiter close() noexcept
    {
      return TcpCloseAwaiter(sockfd_);
    }

  private:
    int sockfd_;
  };

  class TcpServer
  {
  public:
    explicit TcpServer(int port = ::coro::config::kDefaultPort) noexcept
        : TcpServer(nullptr, port) {}
    TcpServer(const char *addr, int port) noexcept;

    TcpAcceptAwaiter accept(int flags = 0) noexcept;

  private:
    // TODO: move to new class Socket?
    int listenfd_;
    int port_;
    sockaddr_in servaddr_;
  };

  class TcpClient
  {
  public:
    TcpClient(const char *addr, int port) noexcept;

    TcpConnectAwaiter connect(int flags = 0) noexcept;

  private:
    int clientfd_;
    int port_;
    sockaddr_in servaddr_;
  };

};