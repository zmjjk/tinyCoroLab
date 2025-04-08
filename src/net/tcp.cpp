#include <exception>

#include "coro/log.hpp"
#include "coro/net/tcp.hpp"
#include "coro/utils.hpp"

namespace coro::net
{
/**
 * @brief TCP服务器构造函数
 * 
 * @param addr 服务器绑定的IP地址，nullptr表示绑定所有可用地址
 * @param port 服务器监听的端口号
 * 
 * 创建一个非阻塞的TCP服务器套接字，并绑定到指定地址和端口
 */
tcp_server::tcp_server(const char* addr, int port) noexcept
{
    // 创建TCP套接字
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(m_listenfd != -1);

    // 设置套接字为非阻塞模式，以便与协程异步IO配合
    coro::utils::set_fd_noblock(m_listenfd);

    // 初始化服务器地址结构
    memset(&m_servaddr, 0, sizeof(m_servaddr));
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port   = htons(port);
    if (addr != nullptr)
    {
        // 将IP地址字符串转换为网络字节序的二进制形式
        if (inet_pton(AF_INET, addr, &m_servaddr.sin_addr.s_addr) < 0)
        {
            log::info("addr invalid");
            std::terminate();
        }
    }
    else
    {
        // 如果未指定地址，则绑定到所有可用地址
        m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    // 将套接字绑定到指定地址和端口
    if (bind(m_listenfd, (sockaddr*)&m_servaddr, sizeof(m_servaddr)) != 0)
    {
        log::info("server bind error");
        std::terminate();
    }

    // 开始监听连接请求
    if (listen(m_listenfd, ::coro::config::kBacklog) != 0)
    {
        log::info("server listen error");
        std::terminate();
    }
}

/**
 * @brief 创建一个接受连接的协程等待器
 * 
 * @param flags 接受连接的标志
 * @return tcp_accept_awaiter 返回一个可以与co_await一起使用的等待器
 * 
 * 该方法是TCP服务器与协程系统集成的关键点，它返回一个等待器对象，
 * 当使用co_await调用此方法时，当前协程会被挂起，直到有新的客户端连接到来。
 */
tcp_accept_awaiter tcp_server::accept(int flags) noexcept
{
    return tcp_accept_awaiter(m_listenfd, flags);
}

/**
 * @brief TCP客户端构造函数
 * 
 * @param addr 要连接的服务器IP地址，nullptr表示连接到本机
 * @param port 要连接的服务器端口号
 * 
 * 创建一个非阻塞的TCP客户端套接字，并准备连接参数
 */
tcp_client::tcp_client(const char* addr, int port) noexcept
{
    // 创建TCP套接字
    m_clientfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(m_clientfd != -1);

    // 设置套接字为非阻塞模式，以便与协程异步IO配合
    utils::set_fd_noblock(m_clientfd);

    // 初始化服务器地址结构
    memset(&m_servaddr, 0, sizeof(m_servaddr));
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port   = htons(port);
    if (addr != nullptr)
    {
        // 将IP地址字符串转换为网络字节序的二进制形式
        if (inet_pton(AF_INET, addr, &m_servaddr.sin_addr.s_addr) < 0)
        {
            assert(false);
        }
    }
    else
    {
        // 如果未指定地址，则连接到本机
        m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
}

/**
 * @brief 创建一个连接到服务器的协程等待器
 * 
 * @param flags 连接的标志
 * @return tcp_connect_awaiter 返回一个可以与co_await一起使用的等待器
 * 
 * 该方法是TCP客户端与协程系统集成的关键点，它返回一个等待器对象，
 * 当使用co_await调用此方法时，当前协程会被挂起，直到连接成功或失败。
 */
tcp_connect_awaiter tcp_client::connect(int flags) noexcept
{
    return tcp_connect_awaiter(m_clientfd, (sockaddr*)&m_servaddr, sizeof(m_servaddr));
}

}; // namespace coro::net
