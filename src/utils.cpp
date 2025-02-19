#include <cassert>
#include <fcntl.h>

namespace coro::utils
{
void set_fd_noblock(int fd) noexcept
{
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);

    flags |= O_NONBLOCK;
    assert(fcntl(fd, F_SETFL, flags) >= 0);
}
}; // namespace coro::utils