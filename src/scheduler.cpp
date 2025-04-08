#include "coro/scheduler.hpp"

namespace coro
{
/**
 * @brief 初始化调度器，创建指定数量的执行上下文
 * 
 * @param ctx_cnt 要创建的执行上下文数量，通常设置为CPU核心数
 * 
 * 实现逻辑:
 * 1. 设置上下文数量成员变量
 * 2. 初始化上下文容器
 * 3. 预分配容器空间以提高性能
 * 4. 创建指定数量的上下文对象
 * 5. 初始化任务分发器
 */
auto scheduler::init_impl(size_t ctx_cnt) noexcept -> void
{
    m_ctx_cnt = ctx_cnt;                      // 保存上下文数量
    m_ctxs    = detail::ctx_container{};      // 初始化上下文容器
    m_ctxs.reserve(m_ctx_cnt);                // 预分配容器空间
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs.emplace_back(std::make_unique<context>()); // 创建上下文对象
    }
    m_dispatcher.init(m_ctx_cnt, &m_ctxs);    // 初始化分发器，传入上下文数量和容器指针
}

/**
 * @brief 启动所有执行上下文
 * 
 * 实现逻辑:
 * 遍历所有上下文对象，调用它们的start方法启动执行线程
 * 每个上下文会创建一个工作线程，开始处理任务队列
 */
auto scheduler::start_impl() noexcept -> void
{
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->start();                   // 启动每个上下文的工作线程
    }
}

/**
 * @brief 控制调度器的运行模式和生命周期
 * 
 * @param long_run_mode 长期运行模式标志
 *        - true: 调度器将持续运行，主线程会阻塞在第一个上下文
 *        - false: 调度器将发送停止信号给所有上下文，然后等待它们结束
 * 
 * 实现逻辑:
 * 根据运行模式选择不同的处理方式
 */
auto scheduler::loop_impl(bool long_run_mode) noexcept -> void
{
    if (long_run_mode)
    {
        join_impl();                          // 长期运行模式：主线程加入第一个上下文
    }
    else
    {
        stop_impl();                          // 短期运行模式：停止所有上下文
    }
}

/**
 * @brief 停止所有执行上下文并等待它们结束
 * 
 * 实现逻辑:
 * 1. 向所有上下文发送停止通知
 * 2. 等待所有上下文的工作线程结束
 * 
 * 这是一个两阶段过程，确保所有上下文都收到停止信号后再等待它们结束
 */
auto scheduler::stop_impl() noexcept -> void
{
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->notify_stop();             // 通知每个上下文停止
    }

    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->join();                    // 等待每个上下文的工作线程结束
    }
}

/**
 * @brief 在长期运行模式下，让主线程加入第一个上下文的工作线程
 * 
 * 实现逻辑:
 * 只加入第一个上下文(索引0)的工作线程，这会阻塞当前线程
 * 直到该上下文的工作线程结束
 * 
 * 注意：这种模式下，只有第一个上下文会被主线程join，
 * 其他上下文的工作线程会继续独立运行
 */
auto scheduler::join_impl() noexcept -> void
{
    m_ctxs[0]->join();                        // 主线程加入第一个上下文
}

/**
 * @brief 提交一个协程任务到适当的执行上下文
 * 
 * @param handle 要提交的协程句柄
 * 
 * 实现逻辑:
 * 1. 使用分发器决定将任务分配给哪个上下文
 * 2. 将任务提交到选定的上下文
 * 
 * 分发策略由dispatcher实现，可能基于负载均衡、轮询等算法
 */
auto scheduler::submit_task_impl(std::coroutine_handle<> handle) noexcept -> void
{
    size_t ctx_id = m_dispatcher.dispatch();  // 获取目标上下文ID
    m_ctxs[ctx_id]->submit_task(handle);      // 将任务提交到选定的上下文
}
}; // namespace coro
