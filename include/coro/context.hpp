/**
 * @file context.hpp
 * @author JiahuiWang
 * @brief lab2b
 * @version 1.0
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "config.h"
#include "coro/engine.hpp"
#include "coro/meta_info.hpp"
#include "coro/task.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab2b, in this part you will use engine to build context. Like
 * equipping soldiers with weapons, context will use engine to execute io task and computation task.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note Do not modify existing functions and class declaration, which may cause the test to not run
 * correctly, but you can change the implementation logic if you need.
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 */

using config::ctx_id;
using std::atomic;
using std::jthread;
using std::make_unique;
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::stop_token;
using std::unique_ptr;

using detail::ginfo;
using detail::linfo;

using engine = detail::engine;

class scheduler;

/**
 * @brief Each context own one engine, it's the core part of tinycoro,
 * which can process computation task and io task
 *
 * context是tinyCoro的核心执行单元，每个context拥有一个engine和一个工作线程
 * 负责任务的接收和执行，是协程任务的实际运行环境
 */
class context
{
    std::atomic<bool> m_job_ready{false};
public:
    /**
     * @brief 构造函数，初始化context并分配唯一ID
     * 通过原子操作从全局计数器获取唯一标识符
     */
    context() noexcept;
    
    /**
     * @brief 默认析构函数
     */
    ~context() noexcept                = default;
    
    /**
     * @brief 禁用拷贝构造函数，防止context被复制
     */
    context(const context&)            = delete;
    
    /**
     * @brief 禁用移动构造函数，防止context被移动
     */
    context(context&&)                 = delete;
    
    /**
     * @brief 禁用拷贝赋值运算符，防止context被复制赋值
     */
    context& operator=(const context&) = delete;
    
    /**
     * @brief 禁用移动赋值运算符，防止context被移动赋值
     */
    context& operator=(context&&)      = delete;

    /**
     * @brief 初始化context
     * 
     * 初始化内部engine并设置线程本地存储变量，
     * 将当前context和engine与工作线程关联
     */
    [[CORO_TEST_USED(lab2b)]] auto init() noexcept -> void;

    /**
     * @brief 清理context资源
     * 
     * 清理engine资源并重置线程本地存储变量
     */
    [[CORO_TEST_USED(lab2b)]] auto deinit() noexcept -> void;

    /**
     * @brief 启动工作线程
     * 
     * 创建一个jthread并在其中执行init、run和deinit流程
     * 工作线程负责执行所有提交到该context的任务
     */
    auto start() noexcept -> void;

    /**
     * @brief 向工作线程发送停止信号
     * 
     * 通知工作线程停止执行，但不会立即终止线程
     * 线程会在处理完当前任务后检查停止信号并退出
     */
    [[CORO_TEST_USED(lab2b)]] auto notify_stop() noexcept -> void;

    /**
     * @brief 等待工作线程停止
     * 
     * 阻塞调用线程直到工作线程完全退出
     */
    inline auto join() noexcept -> void { m_job->join(); }

    /**
     * @brief 提交右值引用任务到context
     * 
     * @param task 要提交的任务(右值引用)
     * 
     * 获取任务的协程句柄，分离任务所有权，然后提交句柄
     */
    inline auto submit_task(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    /**
     * @brief 提交左值引用任务到context
     * 
     * @param task 要提交的任务(左值引用)
     * 
     * 获取任务的协程句柄并提交
     */
    inline auto submit_task(task<void>& task) noexcept -> void { submit_task(task.handle()); }

    /**
     * @brief 提交协程句柄到context
     * 
     * @param handle 要提交的协程句柄
     * 
     * 将协程句柄添加到engine的任务队列中等待执行
     */
    [[CORO_TEST_USED(lab2b)]] auto submit_task(std::coroutine_handle<> handle) noexcept -> void;

    /**
     * @brief 获取context的唯一ID
     * 
     * @return ctx_id context的唯一标识符
     */
    inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

    /**
     * @brief 增加context的引用计数
     * 
     * @param register_cnt 要增加的计数值，默认为1
     * 
     * 用于同步操作，确保context在有任务等待时不会退出
     */
    [[CORO_TEST_USED(lab2b)]] auto register_wait(int register_cnt = 1) noexcept -> void;

    /**
     * @brief 减少context的引用计数
     * 
     * @param register_cnt 要减少的计数值，默认为1
     * 
     * 当等待的任务完成时调用，减少引用计数
     */
    [[CORO_TEST_USED(lab2b)]] auto unregister_wait(int register_cnt = 1) noexcept -> void;

    /**
     * @brief 获取context关联的engine引用
     * 
     * @return engine& 指向engine的引用
     */
    inline auto get_engine() noexcept -> engine& { return m_engine; }

    /**
     * @brief 工作线程的主逻辑
     * 
     * @param token 停止标记，用于检查是否收到停止信号
     * 
     * 循环处理任务队列中的任务和IO事件，直到收到停止信号
     */
    [[CORO_TEST_USED(lab2b)]] auto run(stop_token token) noexcept -> void;

    // 驱动engine从任务队列取出任务并执行
    auto process_work() noexcept -> void;
    
    // 驱动engine执行IO任务
    auto poll_work() noexcept -> void;
    
    // 判断是否没有IO任务以及引用计数是否为0
    auto empty_wait_task() noexcept -> bool;

private:
    /**
     * @brief engine实例，负责实际的任务调度和IO操作
     * 
     * 使用CORO_ALIGN宏确保按缓存行对齐，减少伪共享问题
     */
    CORO_ALIGN engine   m_engine;
    
    /**
     * @brief 工作线程对象
     * 
     * 使用jthread可以在析构时自动发送停止信号并join
     */
    unique_ptr<jthread> m_job;
    
    /**
     * @brief context的唯一标识符
     * 
     * 在构造函数中从全局计数器获取
     */
    ctx_id              m_id;

    // 可能需要添加的其他成员变量:
    // - 引用计数，用于register_wait和unregister_wait
    std::atomic<int>    m_wait_cnt{0};  // 等待计数器，用于跟踪等待的协程数
    
    // - 状态标志，表示context是否正在运行
};

/**
 * @brief 获取当前线程关联的context引用
 * 
 * @return context& 当前线程的context引用
 * 
 * 通过线程本地存储变量linfo.ctx获取当前线程关联的context
 */
inline context& local_context() noexcept
{
    return *linfo.ctx;
}

/**
 * @brief 提交右值引用任务到当前线程的context
 * 
 * @param task 要提交的任务(右值引用)
 * 
 * 获取当前线程的context并提交任务
 */
inline void submit_to_context(task<void>&& task) noexcept
{
    local_context().submit_task(std::move(task));
}

/**
 * @brief 提交左值引用任务到当前线程的context
 * 
 * @param task 要提交的任务(左值引用)
 * 
 * 获取当前线程的context并提交任务
 */
inline void submit_to_context(task<void>& task) noexcept
{
    local_context().submit_task(task.handle());
}

/**
 * @brief 提交协程句柄到当前线程的context
 * 
 * @param handle 要提交的协程句柄
 * 
 * 获取当前线程的context并提交协程句柄
 */
inline void submit_to_context(std::coroutine_handle<> handle) noexcept
{
    local_context().submit_task(handle);
}

}; // namespace coro
