#include <optional>
#include <sys/socket.h>
#include <unistd.h>

#include "coro/net/io_awaiter.hpp"
#include "coro/scheduler.hpp"

namespace coro::net
{
using ::coro::detail::local_engine;
using detail::io_type;

/**
 * @brief 无操作等待器的构造函数
 * 
 * 初始化一个无操作(nop)等待器，用于测试或同步目的
 */
noop_awaiter::noop_awaiter() noexcept
{
    m_info.type = io_type::nop;  // 设置IO类型为无操作
    m_info.cb   = &noop_awaiter::callback;  // 设置完成回调函数

    io_uring_prep_nop(m_urs);  // 准备一个无操作的io_uring请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief 无操作等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码
 */
auto noop_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief TCP接受连接等待器的构造函数
 * 
 * @param listenfd 监听套接字描述符
 * @param flags 接受连接的标志
 */
tcp_accept_awaiter::tcp_accept_awaiter(int listenfd, int flags) noexcept
{
    m_info.type = io_type::tcp_accept;  // 设置IO类型为TCP接受连接
    m_info.cb   = &tcp_accept_awaiter::callback;  // 设置完成回调函数

    // FIXME: this isn't atomic, maybe cause bug?
    io_uring_prep_accept(m_urs, listenfd, nullptr, &len, flags);  // 准备接受连接请求
    io_uring_sqe_set_data(m_urs, &m_info); // old uring version need set data after prep
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief TCP接受连接等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，成功时为新的客户端套接字描述符
 */
auto tcp_accept_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果（新的客户端套接字描述符）
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief TCP读取数据等待器的构造函数
 * 
 * @param sockfd 套接字描述符
 * @param buf 接收数据的缓冲区
 * @param len 缓冲区长度
 * @param flags 接收数据的标志
 */
tcp_read_awaiter::tcp_read_awaiter(int sockfd, char* buf, size_t len, int flags) noexcept
{
    m_info.type = io_type::tcp_read;  // 设置IO类型为TCP读取
    m_info.cb   = &tcp_read_awaiter::callback;  // 设置完成回调函数

    io_uring_prep_recv(m_urs, sockfd, buf, len, flags);  // 准备接收数据请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief TCP读取数据等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，成功时为读取的字节数
 */
auto tcp_read_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果（读取的字节数）
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief TCP写入数据等待器的构造函数
 * 
 * @param sockfd 套接字描述符
 * @param buf 发送数据的缓冲区
 * @param len 要发送的数据长度
 * @param flags 发送数据的标志
 */
tcp_write_awaiter::tcp_write_awaiter(int sockfd, char* buf, size_t len, int flags) noexcept
{
    m_info.type = io_type::tcp_write;  // 设置IO类型为TCP写入
    m_info.cb   = &tcp_write_awaiter::callback;  // 设置完成回调函数

    io_uring_prep_send(m_urs, sockfd, buf, len, flags);  // 准备发送数据请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief TCP写入数据等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，成功时为发送的字节数
 */
auto tcp_write_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果（发送的字节数）
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief TCP关闭连接等待器的构造函数
 * 
 * @param sockfd 要关闭的套接字描述符
 */
tcp_close_awaiter::tcp_close_awaiter(int sockfd) noexcept
{
    m_info.type = io_type::tcp_close;  // 设置IO类型为TCP关闭
    m_info.cb   = &tcp_close_awaiter::callback;  // 设置完成回调函数

    io_uring_prep_close(m_urs, sockfd);  // 准备关闭套接字请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief TCP关闭连接等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，0表示成功
 */
auto tcp_close_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief TCP建立连接等待器的构造函数
 * 
 * @param sockfd 套接字描述符
 * @param addr 目标服务器地址
 * @param addrlen 地址结构体长度
 */
tcp_connect_awaiter::tcp_connect_awaiter(int sockfd, const sockaddr* addr, socklen_t addrlen) noexcept
{
    m_info.type = io_type::tcp_connect;  // 设置IO类型为TCP连接
    m_info.cb   = &tcp_connect_awaiter::callback;  // 设置完成回调函数
    m_info.data = CASTDATA(sockfd);  // 保存套接字描述符，用于回调函数中

    io_uring_prep_connect(m_urs, sockfd, addr, addrlen);  // 准备连接请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief TCP建立连接等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，0表示成功
 */
auto tcp_connect_awaiter::callback(io_info* data, int res) noexcept -> void
{
    if (res != 0)
    {
        data->result = res;  // 如果连接失败，保存错误码
    }
    else
    {
        data->result = static_cast<int>(data->data);  // 连接成功，返回套接字描述符
    }
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

/**
 * @brief 标准输入等待器的构造函数
 * 
 * @param buf 接收数据的缓冲区
 * @param len 缓冲区长度
 * @param flags 读取标志
 */
stdin_awaiter::stdin_awaiter(char* buf, size_t len, int flags) noexcept
{
    m_info.type = io_type::stdin;  // 设置IO类型为标准输入
    m_info.cb   = &stdin_awaiter::callback;  // 设置完成回调函数

    io_uring_prep_read(m_urs, STDIN_FILENO, buf, len, flags);  // 准备从标准输入读取数据请求
    io_uring_sqe_set_data(m_urs, &m_info);  // 关联io_info结构体到SQE
    local_engine().add_io_submit();  // 通知引擎有新的IO请求需要提交
}

/**
 * @brief 标准输入等待器的回调函数
 * 
 * @param data IO信息结构体指针
 * @param res IO操作的结果代码，成功时为读取的字节数
 */
auto stdin_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;  // 保存操作结果（读取的字节数）
    submit_to_context(data->handle);  // 将协程句柄提交到上下文，恢复协程执行
}

}; // namespace coro::net