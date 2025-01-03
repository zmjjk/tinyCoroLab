#pragma once

#include <cassert>
#include <fcntl.h>

namespace coro
{

  void set_fd_noblock(int fd) noexcept;

};