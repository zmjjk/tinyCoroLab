#pragma once

#include <cassert>
#include <cstring>
#include <functional>
#include <liburing.h>      // Linux io_uring 异步 I/O 接口
#include <sys/eventfd.h>   // Linux eventfd 接口，用于事件通知
#ifdef ENABLE_POOLING
    #include <time.h>      // 启用轮询模式时需要的时间相关函数
#endif // ENABLE_POOLING

#include "config.h"
#include "coro/attribute.hpp"

namespace coro::uring
{
/**
 * @brief io_uring 提交队列条目指针类型别名
 * SQE: Submission Queue Entry - 提交队列条目，用于描述要执行的 I/O 操作
 */
using ursptr     = io_uring_sqe*;

/**
 * @brief io_uring 完成队列条目指针类型别名
 * CQE: Completion Queue Entry - 完成队列条目，包含 I/O 操作的结果
 */
using urcptr     = io_uring_cqe*;

/**
 * @brief 处理完成队列条目的回调函数类型
 * 用于在 I/O 操作完成时执行自定义处理逻辑
 */
using urchandler = std::function<void(urcptr)>;

/**
 * @brief io_uring 代理类
 * 
 * 封装 Linux io_uring 异步 I/O 接口，提供更易用的 API
 * io_uring 是 Linux 内核提供的高性能异步 I/O 框架，支持各种 I/O 操作
 */
class uring_proxy
{
public:
    /**
     * @brief 构造函数
     * 
     * 创建 eventfd 用于通知 io_uring 事件
     * eventfd 是一种轻量级的进程间通信机制，这里用于在 I/O 完成时通知应用程序
     */
    uring_proxy() noexcept
    {
        // 必须在构造函数中初始化 eventfd
        m_efd = eventfd(0, 0);
        assert(m_efd >= 0 && "uring_proxy init event_fd failed");
    }

    /**
     * @brief 析构函数
     * 
     * 默认析构函数，资源清理在 deinit() 中完成
     */
    ~uring_proxy() = default;

    /**
     * @brief 初始化 io_uring 实例
     * 
     * @param entry_length 队列长度，决定了可以同时处理的 I/O 请求数量
     */
    auto init(unsigned int entry_length) noexcept -> void
    {
        // 清空参数结构体
        memset(&m_para, 0, sizeof(m_para));
        
#ifdef ENABLE_POOLING
        // 启用 SQPOLL 模式，内核会创建一个专用线程来轮询提交队列
        // 这可以减少系统调用开销，但会增加 CPU 使用率
        m_para.flags |= IORING_SETUP_SQPOLL;
        m_para.sq_thread_idle = config::kSqthreadIdle;  // 轮询线程空闲超时时间
#endif // ENABLE_POOLING

        // 使用参数初始化 io_uring 实例,会创建提交队列(SQ)和完成队列(CQ)，并设置好内核与用户空间的共享内存区域
        /*
        - entry_length ：指定队列的大小（条目数），必须是 2 的幂次方
        - &m_uring ：输出参数，用于存储初始化后的 io_uring 实例
        - &m_para ：输入参数，包含初始化配置选项（如是否启用 SQPOLL 模式等）
        */
        auto res = io_uring_queue_init_params(entry_length, &m_uring, &m_para);
        assert(res == 0 && "uring_proxy init uring failed");
        
        /**
        * @brief 将 eventfd 文件描述符注册到 io_uring 实例
        * 
        * 注册后，每当有 I/O 操作完成时，内核会向该 eventfd 写入数据，
        * 应用程序可以通过监听 eventfd 来获知有新的完成事件到达，
        * 避免频繁轮询完成队列(CQ)，减少 CPU 开销。
        * 
        * @param ring 指向已初始化的 io_uring 实例的指针
        * @param fd 要注册的 eventfd 文件描述符
        * @return 成功返回 0，失败返回负的错误码
        * 
        * @note 注册后，每次 CQE 到达都会触发 eventfd 通知
        * @warning 一个 io_uring 实例只能注册一个 eventfd
        * @see io_uring_unregister_eventfd() 用于取消注册
        */
        res = io_uring_register_eventfd(&m_uring, m_efd);
        assert(res == 0 && "uring_proxy bind event_fd to uring failed");
    }

    /**
     * @brief 释放 io_uring 资源
     * 
     * 关闭 eventfd 并退出 io_uring 队列
     */
    auto deinit() noexcept -> void
    {
        // 注销 eventfd 操作耗时较长，通常不需要调用
        // io_uring_unregister_eventfd(&m_uring);

        // 关闭 eventfd
        close(m_efd);
        m_efd = -1;
        
        // 释放 io_uring 资源
        io_uring_queue_exit(&m_uring);
    }

    /**
     * @brief 检查 io_uring 是否有已完成的 I/O 操作
     * 
     * @note 非阻塞函数，立即返回结果
     * 
     * @return true 有已完成的 I/O 操作
     * @return false 没有已完成的 I/O 操作
     */
    auto peek_uring() noexcept -> bool
    {
        urcptr cqe{nullptr};//初始化为 nullptr 确保安全
        /**
        * @brief 非阻塞地检查完成队列中的 I/O 操作结果
        * 
        * 该函数会检查 io_uring 完成队列(CQ)中是否有已完成的 I/O 操作，
        * 但不会阻塞调用线程。是高性能异步 I/O 编程中的关键函数。
        * 
        * @param ring 指向已初始化的 io_uring 实例的指针
        * @param cqe_ptr 输出参数，用于接收完成队列条目(CQE)的指针
        * @return 0 表示成功获取到 CQE，-EAGAIN 表示没有可用条目，其他负值表示错误
        */
        io_uring_peek_cqe(&m_uring, &cqe);
        return cqe != nullptr;  // 如果 cqe 不为空，表示有已完成的 I/O
    }

    /**
     * @brief 等待指定数量的 I/O 操作完成
     * 
     * @note 阻塞函数，会等待直到有指定数量的 I/O 完成
     * 
     * @param num 等待完成的 I/O 操作数量，默认为 1
     */
    auto wait_uring(int num = 1) noexcept -> void
    {
        urcptr cqe;
        if (num == 1) [[likely]]  // 最常见的情况是等待一个 I/O 完成
        {
            // 等待一个 I/O 完成
            io_uring_wait_cqe(&m_uring, &cqe);
        }
        else
        {
            // 等待多个 I/O 完成
            io_uring_wait_cqe_nr(&m_uring, &cqe, num);
        }
    }

    /**
     * @brief 标记 CQE 条目已处理
     * 
     * 通知内核该 CQE 已被应用程序处理，允许内核重用该条目空间。
     * 
     * @param cqe 要标记的完成队列条目
     */
    inline auto seen_cqe_entry(urcptr cqe) noexcept -> void CORO_INLINE { io_uring_cqe_seen(&m_uring, cqe); }

    /**
     * @brief 获取一个空闲的 SQE 条目
     * 
     * 用于准备新的 I/O 请求
     * 
     * @return 空闲的提交队列条目指针
     */
    inline auto get_free_sqe() noexcept -> ursptr CORO_INLINE { return io_uring_get_sqe(&m_uring); }

    /**
     * @brief 提交所有准备好的 SQE 条目
     * 
     * 将所有准备好的 I/O 请求提交给内核处理
     * 
     * @return 成功提交的 SQE 数量
     */
    inline auto submit() noexcept -> int CORO_INLINE { return io_uring_submit(&m_uring); }

    /**
     * @brief 遍历处理所有已完成的 CQE 条目
     * 
     * 对每个已完成的 I/O 操作执行指定的回调函数
     * 
     * @param f 处理 CQE 的回调函数
     * @param mark_finish 是否自动标记所有 CQE 为已处理
     * @return 处理的 CQE 数量
     */
    auto handle_for_each_cqe(urchandler f, bool mark_finish = false) noexcept -> size_t
    {
        urcptr   cqe;
        unsigned head;
        unsigned i = 0;
        
        // 遍历所有已完成的 CQE
        io_uring_for_each_cqe(&m_uring, head, cqe)
        {
            f(cqe);  // 执行回调函数处理 CQE
            i++;
        };
        
        // 如果需要，标记所有 CQE 为已处理
        if (mark_finish)
        {
            cq_advance(i);
        }
        return i;  // 返回处理的 CQE 数量
    }

    /**
     * @brief 等待 eventfd 通知
     * 
     * 阻塞等待 io_uring 通过 eventfd 发送的通知
     * 
     * @return eventfd 读取到的值
     */
    auto wait_eventfd() noexcept -> uint64_t
    {
        uint64_t u;
        auto     ret = eventfd_read(m_efd, &u);
        assert(ret != -1 && "eventfd read error");
        return u;
    }

    /**
     * @brief 批量获取已完成的 CQE 条目
     * 
     * 一次性获取多个已完成的 I/O 操作结果
     * 
     * @param cqes 存储 CQE 的数组
     * @param num 最多获取的 CQE 数量
     * @return 实际获取的 CQE 数量
     */
    inline auto peek_batch_cqe(urcptr* cqes, unsigned int num) noexcept -> int CORO_INLINE
    {
        return io_uring_peek_batch_cqe(&m_uring, cqes, num);
    }

    /**
     * @brief 向 eventfd 写入数据
     * 
     * 用于手动触发 eventfd 通知
     * 
     * @param num 要写入的值
     */
    inline auto write_eventfd(uint64_t num) noexcept -> void CORO_INLINE
    {
        auto ret = eventfd_write(m_efd, num);
        assert(ret != -1 && "eventfd write error");
    }

    /**
     * @brief 高效地标记多个 CQE 为已处理
     * 
     * 比单独调用 seen_cqe_entry 更高效
     * 
     * @param num 要标记的 CQE 数量
     */
    inline auto cq_advance(unsigned int num) noexcept -> void CORO_INLINE { io_uring_cq_advance(&m_uring, num); }

private:
    int             m_efd{0};    // eventfd 文件描述符
    io_uring_params m_para;      // io_uring 参数
    io_uring        m_uring;     // io_uring 实例
};

}; // namespace coro::uring
