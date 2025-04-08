/**
 * @file engine.hpp
 * @author JiahuiWang
 * @brief lab2a
 * @version 1.0
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/attribute.hpp"
#include "coro/meta_info.hpp"
#include "coro/uring_proxy.hpp"

namespace coro
{
class context;
};

/**
 * @brief Welcome to tinycoro lab2a, in this part you will build the heart of tinycoro����engine by
 * modifing engine.hpp and engine.cpp, please ensure you have read the document of lab2a.
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

namespace coro::detail
{
using std::array;
using std::atomic;
using std::coroutine_handle;
using std::queue;
using uring::urcptr;
using uring::uring_proxy;
using uring::ursptr;

template<typename T>
// multi producer and multi consumer queue
using mpmc_queue = AtomicQueue<T>;

class engine
{
    friend class ::coro::context;

public:
        // 标志位定义
        static constexpr uint64_t task_mask = (0xFFFFF00000000000);
        static constexpr uint64_t io_mask   = (0x00000FFFFF000000);
        static constexpr uint64_t cqe_mask  = (0x0000000000FFFFFF);
    
        static constexpr uint64_t task_flag = (((uint64_t)1) << 44);
        static constexpr uint64_t io_flag   = (((uint64_t)1) << 24);
    
        // 唤醒判断宏
        #define wake_by_task(val) (((val) & engine::task_mask) > 0)
        #define wake_by_io(val)   (((val) & engine::io_mask) > 0)
        #define wake_by_cqe(val)  (((val) & engine::cqe_mask) > 0)
    engine() noexcept { m_id = ginfo.engine_id.fetch_add(1, std::memory_order_relaxed); }

    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

    [[CORO_TEST_USED(lab2a)]] auto init() noexcept -> void;

    [[CORO_TEST_USED(lab2a)]] auto deinit() noexcept -> void;

    /**
     * @brief return if engine has task to run
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] auto ready() noexcept -> bool;

    /**
     * @brief get the free uring sqe entry
     *
     * @return ursptr
     */
    [[CORO_DISCARD_HINT]] auto get_free_urs() noexcept -> ursptr;

    /**
     * @brief return number of task to run in engine
     *
     * @return size_t
     */
    [[CORO_TEST_USED(lab2a)]] auto num_task_schedule() noexcept -> size_t;

    /**
     * @brief fetch one task handle and engine should erase it from its queue
     *
     * @return coroutine_handle<>
     */
    [[CORO_TEST_USED(lab2a)]] [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

    /**
     * @brief submit one task handle to engine
     *
     * @param handle
     */
    [[CORO_TEST_USED(lab2a)]] auto submit_task(coroutine_handle<> handle) noexcept -> void;

    /**
     * @brief this will call schedule() to fetch one task handle and run it
     *
     * @note this function will call clean() when handle is done
     */
    auto exec_one_task() noexcept -> void;

    /**
     * @brief handle one cqe entry
     *
     * @param cqe io_uring cqe entry
     */
    auto handle_cqe_entry(urcptr cqe) noexcept -> void;

    /**
     * @brief submit uring sqe and wait uring finish, then handle
     * cqe entry by call handle_cqe_entry
     *
     */
    [[CORO_TEST_USED(lab2a)]] auto poll_submit() noexcept -> void;

    /**
     * @brief tell engine there has one io to be submitted, engine will record this
     *
     */
    [[CORO_TEST_USED(lab2a)]] auto add_io_submit() noexcept -> void;

    /**
     * @brief return if there has any io to be processed,
     * io include two types: submit_io, running_io
     *
     * @note submit_io: io to be submitted
     * @note running_io: io is submitted but it has't run finished
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] auto empty_io() noexcept -> bool;

    /**
     * @brief return engine unique id
     *
     * @return uint32_t
     */
    inline auto get_id() noexcept -> uint32_t { return m_id; }

    // TODO[lab2a]: Add more function if you need
    auto wake_up(uint64_t val) noexcept -> void;
    auto do_io_submit() noexcept -> void;

private:
    /**
     * @brief 引擎唯一标识符
     * 
     * 在构造函数中通过原子操作从全局计数器获取，确保每个引擎实例拥有唯一ID
     * 用于区分不同的引擎实例，特别是在多线程环境中
     */
    uint32_t m_id;

    /**
     * @brief io_uring 代理对象
     * 
     * 封装了 Linux io_uring 异步 I/O 接口，提供高性能的异步 I/O 操作
     * 负责管理 SQE(提交队列条目)和 CQE(完成队列条目)
     * 在 init() 中初始化，在 deinit() 中释放资源
     */
    uring_proxy m_upxy;

    /**
     * @brief 协程任务队列
     * 
     * 存储待执行的协程句柄，使用无锁队列实现，支持多生产者多消费者模式
     * submit_task() 向队列添加任务，schedule() 从队列获取任务
     * 无锁设计确保在多线程环境中的高性能
     */
    mpmc_queue<coroutine_handle<>> m_task_queue;

    /**
     * @brief 完成队列条目数组
     * 
     * 用于批量获取 io_uring 完成队列中的条目
     * 大小由 config::kQueCap 常量决定，与 io_uring 队列容量一致
     * 在 poll_submit() 中用于存储批量获取的 CQE
     */
    array<urcptr, config::kQueCap> m_urc;

    /**
     * @brief 待提交 IO 操作计数器
     * 
     * 原子变量，记录当前等待提交到 io_uring 的 IO 操作数量
     * add_io_submit() 增加计数，poll_submit() 提交后减少计数
     * 用于 empty_io() 判断是否还有待处理的 IO 操作
     */
    std::atomic<size_t> m_pending_io_submits{0};
    std::atomic<size_t> m_num_io_running{0};
    // TODO[lab2a]: Add more member variables if you need
};

/**
 * @brief return local thread engine
 *
 * @return engine&
 */
inline engine& local_engine() noexcept
{
    return *linfo.egn;
}

}; // namespace coro::detail
