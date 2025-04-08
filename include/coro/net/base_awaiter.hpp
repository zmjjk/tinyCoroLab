#pragma once

#include <coroutine>

#include "coro/context.hpp"
#include "coro/net/io_info.hpp"
#include "coro/uring_proxy.hpp"

namespace coro::net::detail
{

/**
 * @brief 基础IO等待器类，所有具体IO等待器的基类
 * 
 * 该类实现了协程等待器的基本接口，用于处理异步IO操作。
 * 它提供了协程挂起和恢复的基础设施，具体的IO操作由派生类实现。
 */
class base_io_awaiter
{
public:
    /**
     * @brief 构造函数，获取一个可用的io_uring提交队列条目
     * 
     * 从引擎中获取一个空闲的uring提交队列条目(SQE)用于后续的IO操作。
     * 如果没有可用的SQE，表示IO提交速率过高，当前实现会断言失败。
     */
    base_io_awaiter() noexcept : m_urs(coro::detail::local_engine().get_free_urs())
    {
        // TODO: you should no-block wait when get_free_urs return nullptr,
        // this means io submit rate is too high.
        assert(m_urs != nullptr && "io submit rate is too high");
    }

    /**
     * @brief 检查协程是否可以立即恢复执行
     * 
     * 在协程co_await表达式中调用，返回false表示需要挂起协程。
     * 对于IO操作，总是返回false，因为IO操作需要等待完成。
     * 
     * @return 始终返回false，表示需要挂起协程
     */
    constexpr auto await_ready() noexcept -> bool { return false; }

    /**
     * @brief 挂起当前协程
     * 
     * 在协程被挂起时调用，保存协程句柄以便在IO完成后恢复执行。
     * 
     * @param handle 当前协程的句柄
     */
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { m_info.handle = handle; }

    /**
     * @brief 恢复协程执行时获取IO操作结果
     * 
     * 在协程恢复执行时调用，返回IO操作的结果。
     * 
     * @return IO操作的结果代码，通常是操作成功时的字节数或错误码
     */
    auto await_resume() noexcept -> int32_t { return m_info.result; }

protected:
    /**
     * @brief IO操作的信息结构体
     * 
     * 包含协程句柄、操作结果、操作类型、相关数据和回调函数。
     * 派生类需要设置适当的类型和回调函数。
     */
    io_info             m_info;
    
    /**
     * @brief io_uring提交队列条目指针
     * 
     * 指向一个可用的io_uring SQE，用于提交具体的IO操作。
     * 派生类负责使用适当的io_uring_prep_*函数准备此SQE。
     */
    coro::uring::ursptr m_urs;
};

}; // namespace coro::net::detail