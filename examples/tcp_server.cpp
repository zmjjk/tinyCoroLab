#include "coro/context.hpp"
#include "log/log.hpp"
#include "net/tcp.hpp"
using namespace coro;

Task<> server(int port)
{
  log::info("server start in {}", port);
  auto server = net::TcpListener(port);
  int client_fd;
  while ((client_fd = co_await server.accept()) > 0)
  {
    log::info("server receive new connect");
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
