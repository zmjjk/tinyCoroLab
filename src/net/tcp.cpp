#include "net/tcp.hpp"
#include "utils/utils.hpp"

namespace coro::net
{
  TcpListener::TcpListener(int port) noexcept
  {
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd_ != -1);

    set_fd_noblock(listenfd_);

    memset(&servaddr_, 0, sizeof(servaddr_));
    servaddr_.sin_family = AF_INET;
    servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr_.sin_port = htons(port);

    if (bind(listenfd_, (sockaddr *)&servaddr_, sizeof(servaddr_)) != 0)
    {
      assert(false);
    }

    if (listen(listenfd_, ::coro::config::kBacklog) != 0)
    {
      assert(false);
    }
  }

  TcpAcceptAwaiter TcpListener::accept() noexcept
  {
    return TcpAcceptAwaiter(listenfd_);
  }
};