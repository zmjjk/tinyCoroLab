/**
 * @file event.hpp
 * @author JiahuiWang
 * @brief lab4a
 * @version 1.0
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <atomic>
#include <coroutine>
#include <optional>
#include <vector>


#include "coro/attribute.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/context.hpp"
#include "coro/detail/container.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4a, in this part you will build the basic coroutine
 * synchronization component - event by modifing event.hpp and event.cpp. Please ensure
 * you have read the document of lab4a.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 *
 * @note lab4 and lab5 are free designed lab, leave the interfaces that the test case will use,
 * and then, enjoy yourself!
 */
class context;

namespace detail
{
// TODO[lab4a]: Add code that you don't want to use externally in namespace detail
}; // namespace detail

// TODO[lab4a]: This event is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the function set() and wait()'s declaration same with example.
class evevnt_base
{
protected:
    std::atomic<bool>m_is_set{false};
    std::mutex m_waiter_mutex;
    std::vector<std::coroutine_handle<>>m_waiters;
    class awaiter_base{};  
};

/**
 * @brief 泛型事件类，用于协程间同步
 * 
 * @tparam return_type 事件传递的数据类型，默认为void
 * 
 * 事件允许一个协程等待另一个协程发出的信号，并可选择性地传递数据。
 * 当一个协程调用wait()时，如果事件尚未被设置，该协程将被挂起；
 * 当另一个协程调用set()时，所有等待的协程将被恢复执行。
 */
 //模板声明表示定义了一个泛型类 event ，其中 return_type 是一个模板参数，默认类型为 void 。
template<typename return_type = void>
class event
{
    /**
     * @brief 事件等待器，实现协程挂起和恢复的机制
     * 
     * 当协程执行co_await event.wait()时，此等待器控制协程的挂起和恢复。
     * 继承自noop_awaiter以获取基本的awaiter功能。
     */
    struct awaiter : detail::noop_awaiter
    {
        /**
         * @brief 恢复协程执行时返回的值
         * 
         * @return return_type 事件设置的值
         */
        auto await_resume() -> return_type { return {}; }
    };
    // 需要包含optional头文件
public:
    /**
     * @brief 获取事件等待器
     * 
     * 当协程需要等待此事件时调用，返回一个可以与co_await一起使用的等待器。
     * 
     * @return awaiter 事件等待器
     */
    auto wait() noexcept -> awaiter { return {}; } // return awaitable

    /**
     * @brief 设置事件，并传递值
     * 
     * 设置事件为已触发状态，并存储传递的值。
     * 所有等待此事件的协程将被恢复执行。
     * 
     * @tparam value_type 传递值的类型
     * @param value 要传递的值
     */
    template<typename value_type>
    auto set(value_type&& value) noexcept -> void
    {
    }

};


/**
 * @brief 无返回值的事件类特化
 * 
 * 当不需要传递数据时使用的事件特化版本。event 类模板的全特化（full specialization）。
 */
template<>
class event<>
{
public:
    /**
     * @brief 获取事件等待器
     * 
     * @return detail::noop_awaiter 事件等待器
     */
    auto wait() noexcept -> detail::noop_awaiter { return {}; } // return awaitable
    
    /**
     * @brief 设置事件
     * 
     * 设置事件为已触发状态，恢复所有等待此事件的协程。
     */
    auto set() noexcept -> void {}
};

/**
 * @brief 事件的RAII包装器
 * 
 * 在对象生命周期结束时自动设置事件，用于确保事件在特定作用域结束时被触发。
 */
class event_guard
{
    /**
     * @brief 使用的事件类型
     */
    using guard_type = event<>;//guard_type ，它指向特化版本的 event 模板类，不带模板参数（相当于 event<void> ）

public:
    /**
     * @brief 构造函数
     * 
     * @param ev 要管理的事件引用
     */
    event_guard(guard_type& ev) noexcept : m_ev(ev) {}
    
    /**
     * @brief 析构函数
     * 
     * 当对象销毁时自动设置事件
     */
    ~event_guard() noexcept { m_ev.set(); }

private:
    /**
     * @brief 被管理的事件引用
     */
    guard_type& m_ev;
};


}; // namespace coro
