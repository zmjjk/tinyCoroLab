#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "coro/dispatcher.hpp"

namespace coro
{

/**
 * @brief scheduler 负责控制执行上下文(context)的运行和停止
 * 同时使用分发器(dispatcher)决定哪个上下文可以接受新任务
 * 
 * 调度器采用单例模式设计，通过静态方法提供全局访问点
 */
class scheduler
{
    friend context;  // 声明context为友元类，允许context访问scheduler的私有成员

public:
    /**
     * @brief 初始化调度器，创建指定数量的执行上下文
     * 
     * @param ctx_cnt 要创建的上下文数量，默认为系统CPU核心数
     *                如果传入0，则使用系统CPU核心数
     * 
     * 该方法获取调度器单例，并调用其init_impl方法完成实际初始化
     */
    inline static auto init(size_t ctx_cnt = std::thread::hardware_concurrency()) noexcept -> void
    {
        if (ctx_cnt == 0)
        {
            ctx_cnt = std::thread::hardware_concurrency();
        }
        get_instance()->init_impl(ctx_cnt);// 调用单例的初始化方法
    }

    /**
     * @brief 启动所有执行上下文
     * 
     * 获取调度器单例，并调用其start_impl方法启动所有上下文的工作线程
     */
    inline static auto start() noexcept -> void { get_instance()->start_impl(); }

    /**
     * @brief 控制调度器的运行模式
     * 
     * @param long_run_mode 长期运行模式标志
     *        - true: 调度器将持续运行，主线程会阻塞在第一个上下文
     *        - false: 调度器将发送停止信号给所有上下文，然后等待它们结束
     * 
     * 默认值由配置常量kLongRunMode决定
     */
    inline static auto loop(bool long_run_mode = config::kLongRunMode) noexcept -> void
    {
        get_instance()->loop_impl(long_run_mode);
    }

    /**
     * @brief 提交一个右值引用的无返回值任务到调度器
     * 
     * @param task 要提交的任务(右值引用)
     * 
     * 获取任务的协程句柄，分离任务所有权，然后提交句柄
     */
    static inline auto submit(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();  // 获取任务的协程句柄
        task.detach();                // 分离任务所有权，防止任务析构时销毁协程帧
        submit(handle);               // 提交协程句柄
    }

    /**
     * @brief 提交一个左值引用的无返回值任务到调度器
     * 
     * @param task 要提交的任务(左值引用)
     * 
     * 获取任务的协程句柄并提交
     */
    static inline auto submit(task<void>& task) noexcept -> void { submit(task.handle()); }

    /**
     * @brief 提交一个协程句柄到调度器
     * 
     * @param handle 要提交的协程句柄
     * 
     * 获取调度器单例，并调用其submit_task_impl方法完成实际提交
     */
    inline static auto submit(std::coroutine_handle<> handle) noexcept -> void
    {
        get_instance()->submit_task_impl(handle);
    }

private:
    /**
     * @brief 获取调度器单例
     * 
     * @return scheduler* 指向调度器单例的指针
     * 
     * 使用静态局部变量实现线程安全的单例模式
     */
    static auto get_instance() noexcept -> scheduler*
    {
        static scheduler sc;  // 静态局部变量，保证只创建一次
        return &sc;
    }

    // 以下是实际实现函数的声明，定义在scheduler.cpp中

    /**
     * @brief 初始化调度器实现
     * @param ctx_cnt 上下文数量
     */
    auto init_impl(size_t ctx_cnt) noexcept -> void;

    /**
     * @brief 启动所有上下文实现
     */
    auto start_impl() noexcept -> void;

    /**
     * @brief 控制运行模式实现
     * @param long_run_mode 长期运行模式标志
     */
    auto loop_impl(bool long_run_mode) noexcept -> void;

    /**
     * @brief 停止所有上下文实现
     */
    auto stop_impl() noexcept -> void;

    /**
     * @brief 主线程加入第一个上下文实现
     */
    auto join_impl() noexcept -> void;

    /**
     * @brief 提交任务实现
     * @param handle 协程句柄
     */
    auto submit_task_impl(std::coroutine_handle<> handle) noexcept -> void;

private:
    size_t                                              m_ctx_cnt{0};  // 上下文数量
    detail::ctx_container                               m_ctxs;        // 上下文容器
    detail::dispatcher<coro::config::kDispatchStrategy> m_dispatcher;  // 任务分发器
};

/**
 * @brief 提交右值引用任务到调度器或当前上下文
 * 
 * @param task 要提交的任务(右值引用)
 * 
 * 根据长期运行模式配置决定提交目标:
 * - 长期运行模式: 提交到调度器
 * - 非长期运行模式: 提交到当前上下文
 */
inline void submit_to_scheduler(task<void>&& task) noexcept
{
    if (config::kLongRunMode)
    {
        scheduler::submit(std::move(task));
    }
    else
    {
        submit_to_context(std::move(task));
    }
}

/**
 * @brief 提交左值引用任务到调度器或当前上下文
 * 
 * @param task 要提交的任务(左值引用)
 * 
 * 根据长期运行模式配置决定提交目标
 */
inline void submit_to_scheduler(task<void>& task) noexcept
{
    if (config::kLongRunMode)
    {
        scheduler::submit(task.handle());
    }
    else
    {
        submit_to_context(task.handle());
    }
}

/**
 * @brief 提交协程句柄到调度器或当前上下文
 * 
 * @param handle 要提交的协程句柄
 * 
 * 根据长期运行模式配置决定提交目标
 */
inline void submit_to_scheduler(std::coroutine_handle<> handle) noexcept
{
    if (config::kLongRunMode)
    {
        scheduler::submit(handle);
    }
    else
    {
        submit_to_context(handle);
    }
}

}; // namespace coro
