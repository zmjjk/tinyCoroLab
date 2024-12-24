#include "net/tcp.hpp"
#include "utils/utils.hpp"

namespace coro::net
{
  TcpServer::TcpServer(const char *addr, int port) noexcept
  {
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd_ != -1);

    set_fd_noblock(listenfd_);

    memset(&servaddr_, 0, sizeof(servaddr_));
    servaddr_.sin_family = AF_INET;
    servaddr_.sin_port = htons(port);
    if (addr != nullptr)
    {
      if (inet_pton(AF_INET, addr, &servaddr_.sin_addr.s_addr) < 0)
      {
        assert(false && "addr invalid");
      }
    }
    else
    {
      servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(listenfd_, (sockaddr *)&servaddr_, sizeof(servaddr_)) != 0)
    {
      assert(false);
    }

    if (listen(listenfd_, ::coro::config::kBacklog) != 0)
    {
      assert(false);
    }
  }

  TcpAcceptAwaiter TcpServer::accept(int flags) noexcept
  {
    return TcpAcceptAwaiter(listenfd_, flags);
  }

  TcpClient::TcpClient(const char *addr, int port) noexcept
  {
    clientfd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(clientfd_ != -1);

    set_fd_noblock(clientfd_);

    memset(&servaddr_, 0, sizeof(servaddr_));
    servaddr_.sin_family = AF_INET;
    servaddr_.sin_port = htons(port);
    if (addr != nullptr)
    {
      if (inet_pton(AF_INET, addr, &servaddr_.sin_addr.s_addr) < 0)
      {
        assert(false);
      }
    }
    else
    {
      servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  TcpConnectAwaiter TcpClient::connect(int flags) noexcept
  {
    return TcpConnectAwaiter(clientfd_, (sockaddr *)&servaddr_, sizeof(servaddr_));
  }
};