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
  };

  class TcpListener
  {
  public:
    TcpListener(int port = ::coro::config::kDefaultPort) noexcept;

    TcpAcceptAwaiter accept() noexcept;

  private:
    int listenfd_;
    int port_;
    sockaddr_in servaddr_;
  };

};