#pragma once

#include <coroutine>
#include <cstdint>
#include <functional>

namespace coro::net::detail
{
/**
 * @brief 用于将指针转换为无符号整数类型
 * @param data 要转换的指针
 * @return 转换后的无符号整数
 */
#define CASTPTR(data)  reinterpret_cast<uintptr_t>(data)

/**
 * @brief 用于将数据转换为无符号整数类型
 * @param data 要转换的数据
 * @return 转换后的无符号整数
 */
#define CASTDATA(data) static_cast<uintptr_t>(data)

// 前向声明io_info结构体
struct io_info;

/**
 * @brief 使用标准库的协程句柄类型
 */
using std::coroutine_handle;

/**
 * @brief 定义IO操作完成后的回调函数类型
 * @param io_info* IO信息结构体指针
 * @param int IO操作的结果代码
 */
using cb_type = std::function<void(io_info*, int)>;

/**
 * @brief IO操作类型枚举
 */
enum io_type
{
    nop,         ///< 无操作
    tcp_accept,  ///< TCP接受连接操作
    tcp_connect, ///< TCP建立连接操作
    tcp_read,    ///< TCP读取数据操作
    tcp_write,   ///< TCP写入数据操作
    tcp_close,   ///< TCP关闭连接操作
    stdin,       ///< 标准输入操作
    none         ///< 未指定操作
};

/**
 * @brief IO操作信息结构体，用于在协程挂起时保存状态
 */
struct io_info
{
    coroutine_handle<> handle; ///< 协程句柄，用于在IO完成后恢复协程执行
    int32_t            result; ///< IO操作的结果代码
    io_type            type;   ///< IO操作的类型
    uintptr_t          data;   ///< 与IO操作相关的数据，通常是缓冲区指针或文件描述符
    cb_type            cb;     ///< IO操作完成后的回调函数
};

/**
 * @brief 将io_info指针转换为无符号整数
 * @param info io_info结构体指针
 * @return 转换后的无符号整数
 */
inline uintptr_t ioinfo_to_ptr(io_info* info) noexcept
{
    return reinterpret_cast<uintptr_t>(info);
}

/**
 * @brief 将无符号整数转换回io_info指针
 * @param ptr 表示io_info的无符号整数
 * @return 转换后的io_info结构体指针
 */
inline io_info* ptr_to_ioinfo(uintptr_t ptr) noexcept
{
    return reinterpret_cast<io_info*>(ptr);
}

}; // namespace coro::net::detail