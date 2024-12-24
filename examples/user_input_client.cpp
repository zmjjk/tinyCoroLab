#include "coro/context.hpp"
#include "log/log.hpp"
#include "net/tcp.hpp"
using namespace coro;

#define BUFFLEN 10240

Task<> echo(int sockfd)
{
  char buf[BUFFLEN] = {0};
  int ret = 0;
  auto conn = net::TcpConnector(sockfd);

  while (true)
  {
    ret = co_await net::StdinAwaiter(buf, BUFFLEN, 0);
    log::info("receive data from stdin: {}", buf);
    ret = co_await conn.write(buf, ret);
  }
}

Task<> client(const char *addr, int port)
{
  auto client = net::TcpClient(addr, port);
  int ret = 0;
  int sockfd = 0;
  sockfd = co_await client.connect();
  assert(sockfd > 0 && "connect error");

  submit_task(echo(sockfd));

  char buf[BUFFLEN] = {0};
  auto conn = net::TcpConnector(sockfd);
  while ((ret = co_await conn.read(buf, BUFFLEN)) > 0)
  {
    log::info("receive data from net: {}", buf);
  }

  ret = co_await conn.close();
  assert(ret == 0);
}

int main(int argc, char const *argv[])
{
  /* code */
  Context ctx;
  ctx.submit_task(client("localhost", 8000));
  ctx.start();
  ctx.join();
  return 0;
}
