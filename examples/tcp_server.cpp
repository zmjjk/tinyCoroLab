#include "coro/context.hpp"
#include "log/log.hpp"
#include "net/tcp.hpp"
using namespace coro;

Task<> session(int fd)
{
  char buf[10240] = {0};
  auto client = net::TcpAcceptor(fd);
  int ret = 0;
  while ((ret = co_await client.read(buf, 10240)) > 0)
  {
    log::info("client {} receive data: {}", fd, buf);
    ret = co_await client.write(buf, ret);
  }
}

Task<> server(int port)
{
  log::info("server start in {}", port);
  auto server = net::TcpListener(port);
  int client_fd;
  while ((client_fd = co_await server.accept()) > 0)
  {
    log::info("server receive new connect");
    submit_task(session(client_fd));
  }
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx;
  ctx.submit_task(server(8000));
  ctx.start();
  ctx.join();

  return 0;
}
