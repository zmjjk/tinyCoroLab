#include "utils/utils.hpp"

namespace coro
{
  void set_fd_noblock(int fd) noexcept
  {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
      assert(false);
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
      assert(false);
    }
  }

};