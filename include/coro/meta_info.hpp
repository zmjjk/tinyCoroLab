#pragma once

#include <atomic>

#include "coro/attribute.hpp"

namespace coro
{
/**
 * @brief 前向声明 context 类
 * 避免循环依赖，因为 context 类会使用 detail 命名空间中的内容
 */
class context;
}; // namespace coro

namespace coro::detail
{
/**
 * @brief 使用 config 命名空间中的 ctx_id 类型
 * ctx_id 通常是一个整数类型，用于唯一标识一个 context 实例
 */
using config::ctx_id;

/**
 * @brief 使用 std 命名空间中的 atomic 模板
 * 用于线程安全的原子操作
 */
using std::atomic;

/**
 * @brief 前向声明 engine 类
 * engine 是协程执行引擎，负责调度和执行协程任务
 */
class engine;

/**
 * @brief 存储线程本地变量的结构体
 * 
 * 使用 CORO_ALIGN 宏确保按缓存行对齐，减少伪共享问题
 * 每个线程都有自己独立的 local_info 实例
 */
struct CORO_ALIGN local_info
{
    /**
     * @brief 指向当前线程关联的 context 对象的指针
     * 默认初始化为 nullptr，表示尚未关联任何 context
     */
    context* ctx{nullptr};
    
    /**
     * @brief 指向当前线程关联的 engine 对象的指针
     * 默认初始化为 nullptr，表示尚未关联任何 engine
     */
    engine*  egn{nullptr};
    // TODO: Add more local var
};

/**
 * @brief 存储线程间共享变量的结构体
 * 
 * 包含所有线程共享的全局状态，使用原子变量确保线程安全
 */
struct global_info
{
    /**
     * @brief 原子计数器，用于生成唯一的 context ID
     * 初始值为 0，每次创建新的 context 时递增
     */
    atomic<ctx_id>   context_id{0};
    
    /**
     * @brief 原子计数器，用于生成唯一的 engine ID
     * 初始值为 0，每次创建新的 engine 时递增
     */
    atomic<uint32_t> engine_id{0};
    // TODO: Add more global var
};

/**
 * @brief 线程本地存储变量，每个线程有独立的实例
 * 
 * 使用 thread_local 关键字确保每个线程拥有自己的 linfo 副本
 * 用于快速访问当前线程的 context 和 engine
 */
inline thread_local local_info linfo;

/**
 * @brief 全局共享信息实例
 * 
 * 所有线程共享的单一 global_info 实例
 * 用于管理全局状态和生成唯一标识符
 */
inline global_info             ginfo;
}; // namespace coro::detail