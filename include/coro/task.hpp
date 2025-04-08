/**
 * @file task.hpp
 * @author JiahuiWang
 * @brief lab1 - 协程任务核心实现
 * @version 1.1 (根据注释请求调整)
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
 #pragma once

 #include <cassert>
 #include <coroutine>  // C++20协程核心库
 #include <cstdint>    // 用于 uint8_t
 #include <stdexcept>  // 用于 std::exception_ptr
 #include <utility>    // 用于 std::move, std::exchange
 
 #include "coro/attribute.hpp"          // 自定义属性宏 (例如 CORO_AWAIT_HINT)
 #include "coro/detail/container.hpp" // 用于存储返回值的容器 (假设存在)
 
 namespace coro
 {
 // 前向声明 task 类模板，告知编译器存在这个类
 // 默认返回类型为 void
 template<typename return_type = void>
 class task;
 
 namespace detail
 {
 // --- Start promise_base 定义 ---
 
 /**
  * @brief 协程状态枚举类
  *
  * 表示协程是正常运行、已被分离、还是处于无效状态
  */
  enum class coro_status : uint8_t
  {
     normal, // 正常状态，由 task 对象管理生命周期
     detach, // 分离状态，生命周期由执行引擎或调用者通过 clean() 管理
     none    // 无效或未初始化状态
  };
 
 /**
  * @brief 协程 Promise 基类，包含所有协程 Promise 通用的成员和逻辑
  *
  * 每个协程对象内部都有一个 Promise 对象，用于：
  * 1. 关联协程句柄 (coroutine_handle)
  * 2. 控制协程的初始和最终挂起行为 (initial_suspend, final_suspend)
  * 3. 处理协程的返回值或异常 (在派生类中实现)
  * 4. 存储与父协程的关联 (m_parent)
  * 5. 追踪协程状态 (m_statu)
  */
 struct promise_base
 {
     promise_base() noexcept = default; // 默认构造函数
     ~promise_base()         = default; // 默认析构函数
 
     /**
      * @brief 设置协程状态
      * @param state 新的状态 (normal 或 detach)
      * @note 调用关系: 通常在 task::detach() 中被调用以设置为 detach。
      */
     inline auto set_status(coro_status state) -> void { m_statu = state; }
 
     /**
      * @brief 获取当前协程状态
      * @return coro_status 当前的状态
      * @note 调用关系: 在 clean() 函数中被调用，用于判断是否需要销毁协程。
      */
     inline auto get_status() const noexcept -> coro_status { return m_statu; }
 
     /**
      * @brief 检查协程是否处于分离状态
      * @return true 如果状态是 detach
      * @note 调用关系: 可能会在 clean() 或其他管理逻辑中使用 (当前 clean 中直接用 get_status)
      */
     inline auto is_detach() -> bool { return m_statu == coro_status::detach; }
 
     // 协程状态成员变量，默认为 normal
     coro_status m_statu = coro_status::normal;
 
     // 存储父协程句柄，用于 final_suspend 时恢复父协程
     std::coroutine_handle<> m_parent = nullptr; // 初始化为空
 
     /**
      * @brief 初始挂起点 Awaitable - 协程创建时的行为
      *
      * 当协程函数被调用并创建协程帧后，会立即 co_await 这个 awaitable。
      * @return std::suspend_always 表示协程创建后立即挂起，不执行任何代码，
      *         将控制权返回给协程的创建者（调用者）。
      *         这使得协程可以惰性启动，由调度器或调用者决定何时 resume()。
      * @note 调用关系: 由 C++ 协程框架在协程对象创建后隐式调用。
      */
     constexpr auto initial_suspend() noexcept { return std::suspend_always{}; }
 
     /**
      * @brief 最终挂起点 Awaiter - 协程结束时的行为
      *
      * 当协程执行完最后一条语句或遇到 co_return 时，在销毁前会 co_await 这个 awaitable。
      * 这个 awaiter 决定了协程结束后控制权转移到哪里。
      */
     struct final_awaiter {
         /**
          * @brief 检查是否需要挂起 (final_awaiter 总需要挂起以转移控制权)
          * @return false 总是返回 false，表示需要调用 await_suspend。
          * @note 调用关系: 由 C++ 协程框架在 co_await final_awaiter 时隐式调用。
          */
         constexpr bool await_ready() const noexcept { return false; }
 
         /**
          * @brief 执行挂起操作，并决定恢复哪个协程
          * @tparam PromiseType 协程的 Promise 类型
          * @param finished_coroutine 刚刚执行完毕的当前协程的句柄
          * @return 返回下一个应该被恢复执行的协程句柄。
          * @note 调用关系: 由 C++ 协程框架在 await_ready 返回 false 后隐式调用。
          */
         template<typename PromiseType>
         auto await_suspend(std::coroutine_handle<PromiseType> finished_coroutine) noexcept -> std::coroutine_handle<>
         {
             // 1. 获取当前完成协程的 promise 对象
             auto& promise = finished_coroutine.promise();
             // 2. 从 promise 中取出之前存储的父协程句柄
             auto parent_handle = promise.m_parent;
 
             // 3. 决定控制流
             if (parent_handle) {
                 // 如果 m_parent 有效 (即这个协程是被另一个协程 co_await 的)，
                 // 返回父协程的句柄。框架会 resume 这个父协程，
                 // 使其从 co_await 子协程 的地方继续执行。
                 return parent_handle;
             } else {
                 // 如果 m_parent 为空 (例如，协程是被 detach 的，
                 // 或者是由非协程代码直接启动和管理的)，
                 // 返回 std::noop_coroutine()。这表示不恢复任何特定协程，
                 // 控制权返回给最后调用 resume() 的地方 (通常是调度器或事件循环)。
                 return std::noop_coroutine();
             }
         }
 
         /**
          * @brief await_suspend 返回有效句柄后，此函数不会被调用
          * @note 调用关系: 由 C++ 协程框架在 await_suspend 返回 void 或 std::noop_coroutine()
          *         并且协程需要恢复时隐式调用 (但在此 final_awaiter 设计中通常不执行)。
          */
         constexpr void await_resume() const noexcept {}
     }; // end struct final_awaiter
 
     /**
      * @brief 最终挂起点接口函数
      *
      * 当协程自然结束或 co_return 时，协程框架会调用此函数获取 final_awaiter 对象。
      * @return final_awaiter 我们自定义的awaiter，用于实现结束后的控制流转移。
      * @note 调用关系: 由 C++ 协程框架在协程执行完毕后隐式调用。
      */
     [[CORO_TEST_USED(lab1)]] // 标记此函数在 lab1 测试中使用
     auto final_suspend() noexcept -> final_awaiter
     {
         return final_awaiter{}; // 返回自定义 awaiter 的实例
     }
 
 #ifdef DEBUG
 public:
     int promise_id{0}; // 用于调试目的的 ID
 #endif // DEBUG
 }; // end struct promise_base
 
 // --- End promise_base 定义 ---
 
 
 // --- Start promise<return_type> 定义 (带返回值版本) ---
 
 /**
  * @brief 带返回值的协程 Promise 实现
  *
  * 继承自 promise_base (获取通用调度逻辑) 和 container<return_type> (获取返回值存储能力)。
  * @tparam return_type 协程的返回值类型 (非 void)
  */
 template<typename return_type>
 struct promise final : public promise_base, public container<return_type>
 {
 public:
     using task_type        = task<return_type>; // 定义关联的 task 类型
     using coroutine_handle = std::coroutine_handle<promise<return_type>>; // 定义协程句柄类型
 
 #ifdef DEBUG
     // 调试用构造函数
     template<typename... Args>
     promise(int id, Args&&... args) noexcept : container<return_type>(std::forward<Args>(args)...) // 调用基类构造
     {
         promise_id = id;
     }
 #endif // DEBUG
     promise() noexcept {} // 默认构造
     promise(const promise&)             = delete; // 禁止拷贝构造
     promise(promise&& other)            = delete; // 禁止移动构造
     promise& operator=(const promise&)  = delete; // 禁止拷贝赋值
     promise& operator=(promise&& other) = delete; // 禁止移动赋值
     ~promise()                          = default; // 默认析构
 
     /**
      * @brief 获取与此 Promise 关联的协程返回对象 (task)
      *
      * 当协程函数第一次被调用时，框架会创建 Promise 对象，并调用此函数
      * 生成最终返回给调用者的 task 对象。
      * @return task<return_type> 一个新的 task 对象，持有当前协程的句柄。
      * @note 调用关系: 由 C++ 协程框架在协程创建时隐式调用。
      */
     auto get_return_object() noexcept -> task_type; // 实现通常在 detail 命名空间末尾
 
     // 注意：return_value 实现在 container<return_type> 中提供或需要在这里实现：
     // template<typename U = return_type>
     // auto return_value(U&& value) noexcept(...) -> std::enable_if_t<!std::is_void_v<U>> {
     //     this->set_value(std::forward<U>(value)); // 假设 container 有 set_value
     // }
 
     /**
      * @brief 处理协程中未捕获的异常
      *
      * 当协程内部抛出异常且未被捕获时，框架会调用此函数。
      * @note 调用关系: 由 C++ 协程框架在发生未捕获异常时隐式调用。
      * @note 实现: 调用 container 的 set_exception 方法 (假设存在) 来存储异常。
      */
      auto unhandled_exception() noexcept -> void { this->set_exception(); } // 假设 container 有 set_exception
 }; // end struct promise<return_type>
 
 // --- End promise<return_type> 定义 ---
 
 
 // --- Start promise<void> 定义 (void 返回值特化版本) ---
 
 /**
  * @brief 无返回值 (void) 的协程 Promise 特化实现
  *
  * 继承自 promise_base，但不继承 container，因为没有值需要存储。
  */
 template<>
 struct promise<void> : public promise_base
 {
     using task_type        = task<void>; // 定义关联的 task 类型
     using coroutine_handle = std::coroutine_handle<promise<void>>; // 定义协程句柄类型
 
 #ifdef DEBUG
     template<typename... Args>
     promise(int id, Args&&... args) noexcept
     {
         promise_id = id;
     }
 #endif // DEBUG
     promise() noexcept                  = default; // 默认构造
     promise(const promise&)             = delete; // 禁止拷贝构造
     promise(promise&& other)            = delete; // 禁止移动构造
     promise& operator=(const promise&)  = delete; // 禁止拷贝赋值
     promise& operator=(promise&& other) = delete; // 禁止移动赋值
     ~promise()                          = default; // 默认析构
 
     /**
      * @brief 获取与此 Promise 关联的协程返回对象 (task)
      * @return task<void> 一个新的 task 对象，持有当前协程的句柄。
      * @note 调用关系: 由 C++ 协程框架在协程创建时隐式调用。
      */
     auto get_return_object() noexcept -> task_type; // 实现通常在 detail 命名空间末尾
 
     /**
      * @brief 处理协程中的 `co_return;` 语句 (无返回值)
      * @note 调用关系: 由 C++ 协程框架在遇到 `co_return;` 时隐式调用。
      */
     constexpr auto return_void() noexcept -> void {}
 
     /**
      * @brief 处理协程中未捕获的异常
      * @note 调用关系: 由 C++ 协程框架在发生未捕获异常时隐式调用。
      * @note 实现: 将当前异常存储在 m_exception_ptr 中。
      */
     auto unhandled_exception() noexcept -> void { m_exception_ptr = std::current_exception(); }
 
     /**
      * @brief 获取协程执行结果 (对于 void 返回类型，主要用于检查异常)
      *
      * 如果协程执行过程中有未捕获的异常，调用此函数会重新抛出该异常。
      * @note 调用关系: 通常在 task 的 awaiter 的 await_resume() 中被调用，
      *         以在 co_await 完成后传播异常。
      */
     auto result() -> void
     {
         if (m_exception_ptr)
         {
             std::rethrow_exception(m_exception_ptr);
         }
     }
 
 private:
     std::exception_ptr m_exception_ptr{nullptr}; // 用于存储未捕获的异常
 }; // end struct promise<void>
 
 // --- End promise<void> 定义 ---
 
 } // namespace detail
 
 
 // --- Start task<return_type> 定义 ---
 
 /**
  * @brief 协程任务类模板，代表一个异步操作或计算过程
  *
  * task 对象封装了一个协程句柄 (coroutine_handle)，提供了管理协程生命周期、
  * 获取结果以及使其可被 co_await 的接口。
  * @tparam return_type 协程的返回值类型
  */
 template<typename return_type>
 class [[CORO_AWAIT_HINT]] task // CORO_AWAIT_HINT 可能是用于IDE提示的宏
 {
 public:
     // 类型别名，方便使用
     using task_type        = task<return_type>;
     using promise_type     = detail::promise<return_type>; // 关联的 Promise 类型
     using coroutine_handle = std::coroutine_handle<promise_type>; // 具体的协程句柄类型
 
     /**
      * @brief task 的基础 Awaitable 实现
      *
      * 当 `co_await task_object;` 时，编译器会调用 `task_object.operator co_await()`
      * 来获取一个 Awaitable 对象 (通常是这个 base 的派生类)。
      * 然后框架会依次调用 Awaitable 的 `await_ready`, `await_suspend`, `await_resume`。
      */
     struct awaitable_base
     {
         // 持有被等待的 task 的协程句柄
         std::coroutine_handle<promise_type> m_coroutine{nullptr};
 
         // 构造函数
         awaitable_base(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}
 
         /**
          * @brief 检查被等待的协程是否已经完成，决定是否需要挂起等待者
          * @return true 如果协程句柄为空或协程已执行完毕 (done)，表示无需挂起，直接调用 await_resume。
          * @return false 如果协程尚未完成，表示需要挂起等待者，并调用 await_suspend。
          * @note 调用关系: 由 C++ 协程框架在 co_await task 时隐式调用。
          */
         auto await_ready() const noexcept -> bool { return !m_coroutine || m_coroutine.done(); }
 
         /**
          * @brief 在等待者 (父协程) 挂起时执行的操作
          * @param awaiting_coroutine 发起 co_await 的那个协程 (父协程) 的句柄
          * @return 返回接下来应该恢复执行的协程句柄。
          * @note 调用关系: 由 C++ 协程框架在 await_ready 返回 false 后隐式调用。
          * @note 实现:
          *   1. 检查子协程 (m_coroutine) 是否有效。
          *   2. 将父协程句柄 (awaiting_coroutine) 存入子协程的 promise.m_parent。
          *   3. 如果子协程已完成 (不常见但可能)，直接返回父协程句柄 (立即恢复父协程)。
          *   4. 否则，返回子协程句柄 (m_coroutine)，将控制权转移给子协程执行。
          */
         auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> std::coroutine_handle<>
         {
             // 处理无效句柄的情况
             if (!m_coroutine) [[unlikely]] {
                 // 如果子协程句柄无效，不应挂起父协程，
                 // 返回 noop 表示控制权立刻回到父协程（并可能立即调用 await_resume）
                 // 或者根据设计可以返回 awaiting_coroutine
                 return std::noop_coroutine();
             }
 
             // 1. 获取子协程的 promise
             auto& child_promise = m_coroutine.promise();
             // 2. 设置父子关系：子协程结束后应该恢复 awaiting_coroutine (父协程)
             child_promise.m_parent = awaiting_coroutine;
 
             // 3. 决定执行流：
             if (m_coroutine.done()) {
                 // 如果子协程在 co_await 之前就已经完成了 (理论上不该发生，除非协程创建后未暂停或已被执行)
                 // 那么不需要挂起父协程，直接返回父协程句柄
                 return awaiting_coroutine;
             } else {
                 // 正常情况：子协程尚未完成，返回子协程句柄，
                 // 协程框架将 resume 子协程，开始执行或继续执行子协程的代码。
                 return m_coroutine;
             }
         }
 
         // await_resume() 在派生类中实现，用于获取结果
     }; // end struct awaitable_base
 
     // --- task 类的成员函数 ---
 
     /**
      * @brief 默认构造函数，创建一个空的 task 对象 (不持有任何协程)
      */
     task() noexcept : m_coroutine(nullptr) {}
 
     /**
      * @brief 从协程句柄构造 task 对象
      * @param handle 协程句柄 (通常由 promise.get_return_object() 提供)
      * @note 这是 task 对象获取其管理的协程的主要方式。
      */
     explicit task(coroutine_handle handle) : m_coroutine(handle) {}
 
     // 禁止拷贝构造，task 对协程句柄拥有唯一所有权
     task(const task&) = delete;
 
     /**
      * @brief 移动构造函数
      *
      * 从另一个 task 对象转移协程句柄的所有权。
      * @param other 被移动的 task 对象 (其句柄将被置空)
      */
     task(task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}
 
     /**
      * @brief 析构函数
      *
      * 如果 task 对象仍然持有有效的协程句柄 (即未被 detach)，
      * 则在此销毁协程帧，释放资源 (RAII)。
      * @note 调用关系: 在 task 对象生命周期结束时自动调用。
      */
     ~task()
     {
         // 只有当 m_coroutine 非空时才销毁 (detach 后会变空)
         if (m_coroutine != nullptr)
         {
             // 调用 destroy() 释放协程帧内存
             m_coroutine.destroy();
         }
     }
 
     // 禁止拷贝赋值
     auto operator=(const task&) -> task& = delete;
 
     /**
      * @brief 移动赋值运算符
      *
      * 转移协程句柄的所有权。
      * @param other 被移动的 task 对象
      * @return *this
      */
     auto operator=(task&& other) noexcept -> task&
     {
         if (std::addressof(other) != this) // 防止自赋值
         {
             // 先销毁当前持有的句柄 (如果存在)
             if (m_coroutine != nullptr)
             {
                 m_coroutine.destroy();
             }
             // 从 other 转移句柄所有权，并将 other 的句柄置空
             m_coroutine = std::exchange(other.m_coroutine, nullptr);
         }
         return *this;
     }
 
     /**
      * @brief 检查 task 是否已就绪 (协程是否已执行完毕或 task 对象为空)
      * @return true 如果 task 为空或其协程已完成
      * @note 调用关系: 可能被调度器或等待者用来检查状态。
      */
     auto is_ready() const noexcept -> bool { return m_coroutine == nullptr || m_coroutine.done(); }
 
     /**
      * @brief 恢复 (或启动) 关联协程的执行
      * @return true 如果协程恢复后仍未执行完毕
      * @return false 如果协程已经执行完毕或 task 为空
      * @note 调用关系: 通常由调度器或 co_await 的 await_suspend 调用。
      */
     auto resume() -> bool
     {
         // 检查句柄有效且未完成
         if (m_coroutine && !m_coroutine.done())
         {
             m_coroutine.resume(); // 恢复协程执行
         }
         // 返回协程是否 *仍然* 未完成
         return m_coroutine && !m_coroutine.done();
     }
 
     /**
      * @brief 主动销毁关联的协程句柄
      * @return true 如果成功销毁 (句柄之前有效)
      * @return false 如果句柄原本就为空
      * @note 调用关系: 通常不直接调用，由析构函数或 clean() 管理。
      *         但提供此接口允许外部显式销毁。
      */
     auto destroy() -> bool
     {
         if (m_coroutine != nullptr)
         {
             m_coroutine.destroy();
             m_coroutine = nullptr; // 置空，防止二次销毁
             return true;
         }
         return false;
     }
 
     /**
      * @brief 分离 (detach) 任务
      *
      * 使当前 task 对象放弃对协程句柄的所有权，并将协程状态标记为 detach。
      * 分离后，协程的生命周期不再由该 task 对象管理，通常由执行引擎负责清理。
      * 同时，清除协程 promise 中存储的父协程句柄，防止结束后恢复父协程。
      * @note 调用关系: 通常由执行引擎在接管右值 task 后调用，或用户显式调用。
      */
     [[CORO_TEST_USED(lab1)]] // 标记在 lab1 测试中使用
     auto detach() -> void
     {
         // 断言确保 detach 时句柄是有效的
         assert(m_coroutine != nullptr && "detach func expected no-nullptr coroutine_handler");
 
         // 1. 获取 promise 对象
         auto& promise = m_coroutine.promise();
         // 2. 将协程状态设置为 detach
         promise.set_status(detail::coro_status::detach);
         // 3. 清除指向父协程的指针，断开返回路径
         promise.m_parent = nullptr;
         // 4. task 对象释放对协程句柄的所有权 (将其置空)
         // 这样 task 对象析构时就不会 destroy 协程帧了。
         m_coroutine = nullptr;
     }
 
     /**
      * @brief 实现 `co_await task_object;` (当 task_object 是左值引用时)
      *
      * 返回一个 awaitable 对象，其 await_resume 会获取协程结果。
      * @return awaitable 对象实例
      * @note 调用关系: 由 C++ 编译器在遇到 `co_await task_object;` 时调用。
      */
     auto operator co_await() const& noexcept // 对 const 左值进行 co_await
     {
         // 定义具体的 awaitable 类型，继承自 awaitable_base
         struct awaitable : public awaitable_base
         {
             // 使用基类构造函数
             using awaitable_base::awaitable_base;
 
             /**
              * @brief 协程恢复执行时调用，用于获取 co_await 表达式的结果
              * @return 协程的返回值 (通过 promise().result() 获取)
              * @note 调用关系: 由 C++ 协程框架在 await_suspend 返回并需要恢复时，
              *         或者 await_ready 返回 true 时隐式调用。
              * @note 实现: 调用 promise 的 result() 方法获取结果。如果协程有异常，
              *         result() 应该重新抛出异常。
              */
             auto await_resume() -> decltype(auto) {
                  // 从 promise 获取结果，这里会自动处理 void 和非 void 情况
                  return this->m_coroutine.promise().result();
             }
         };
         // 返回 awaitable 实例，传入当前 task 的协程句柄
         return awaitable{m_coroutine};
     }
 
     /**
      * @brief 实现 `co_await std::move(task_object);` (当 task_object 是右值引用时)
      *
      * 返回一个 awaitable 对象，其 await_resume 会移动协程结果 (如果可能)。
      * @return awaitable 对象实例
      * @note 调用关系: 由 C++ 编译器在遇到 `co_await std::move(task_object);` 时调用。
      */
     auto operator co_await() const&& noexcept // 对 const 右值进行 co_await
     {
         // 定义具体的 awaitable 类型
         struct awaitable : public awaitable_base
         {
             using awaitable_base::awaitable_base;
 
             /**
              * @brief 获取 co_await 表达式的结果 (尝试移动)
              * @return 协程的返回值 (通过移动 promise 再调用 result() 获取)
              * @note 调用关系: 同上。
              * @note 实现: 通过 std::move 获取 promise 的右值引用，然后调用 result()。
              *         这允许 result() 的实现有机会移动返回值（如果返回值类型支持移动）。
              */
             auto await_resume() -> decltype(auto) {
                  // 尝试移动结果
                  return std::move(this->m_coroutine.promise()).result();
             }
         };
         // 返回 awaitable 实例
         return awaitable{m_coroutine};
     }
 
     /**
      * @brief 获取关联 Promise 对象的左值引用
      */
     auto promise() & -> promise_type& { return m_coroutine.promise(); }
 
     /**
      * @brief 获取关联 Promise 对象的 const 左值引用
      */
     auto promise() const& -> const promise_type& { return m_coroutine.promise(); }
 
     /**
      * @brief 获取关联 Promise 对象的右值引用 (允许移动)
      */
     auto promise() && -> promise_type&& { return std::move(m_coroutine.promise()); }
 
     /**
      * @brief 获取协程句柄的左值引用
      */
     auto handle() & -> coroutine_handle { return m_coroutine; }
 
     /**
      * @brief 获取协程句柄并转移所有权 (将 task 内部句柄置空)
      */
     auto handle() && -> coroutine_handle { return std::exchange(m_coroutine, nullptr); }
 
 private:
     coroutine_handle m_coroutine{nullptr}; // 存储关联的协程句柄
 }; // end class task
 
 // --- End task<return_type> 定义 ---
 
 
 // --- Start clean() 函数定义 ---
 
 // 定义一个通用的协程句柄别名 (基于 promise_base)
 using coroutine_handle_base = std::coroutine_handle<detail::promise_base>;
 
 /**
  * @brief 清理已完成的协程句柄资源
  *
  * 这个函数由外部（通常是执行引擎或调度器）调用，用于安全地销毁协程帧，
  * 特别是对于已经 detach 的协程。
  * @param handle 通用协程句柄 (std::coroutine_handle<>)
  * @note 调用关系: 由执行引擎在确认协程完成且需要清理时调用。
  */
 [[CORO_TEST_USED(lab1)]] // 标记在 lab1 测试中使用
 inline auto clean(std::coroutine_handle<> handle) noexcept -> void
 {
     // 检查句柄是否有效
     if (!handle) { return; }
 
     // 1. 将通用句柄转换为基于 promise_base 的句柄，以便访问 promise 成员
     auto specific_handle = coroutine_handle_base::from_address(handle.address());
 
     // 2. 获取协程的 Promise 对象
     auto& promise = specific_handle.promise();
 
     // 3. 检查协程状态
     if (promise.get_status() == detail::coro_status::detach) {
         // 如果协程处于 detach 状态，说明其生命周期由引擎管理。
         // 此时应该由 clean 函数负责销毁。
         // （我们假设调用 clean 时协程已经完成，所以不再需要检查 handle.done()，
         //  但检查一下更安全）
         if (handle.done()) { // 确保已完成
              handle.destroy();
         } else {
              // 理论上不应为未完成的协程调用 clean，可以加断言或日志
              assert(false && "clean() called on a non-done detached coroutine!");
         }
     }
     // else (status is normal):
     // 如果协程状态是 normal，说明它仍然由某个 task 对象管理 (RAII)。
     // clean 函数不应销毁它，让 task 对象的析构函数来处理。
 }
 
 // --- End clean() 函数定义 ---
 
 
 namespace detail
 {
 // --- Start get_return_object 实现 ---
 
 /**
  * @brief promise<return_type>::get_return_object() 的实现
  * @return task<return_type>
  * @note 调用关系: 在 promise<return_type> 构造时被框架调用。
  */
 template<typename return_type>
 inline auto promise<return_type>::get_return_object() noexcept -> task<return_type>
 {
     // 从当前 promise 对象创建对应的协程句柄，并用此句柄构造 task 对象
     return task<return_type>{coroutine_handle::from_promise(*this)};
 }
 
 /**
  * @brief promise<void>::get_return_object() 的实现
  * @return task<> (即 task<void>)
  * @note 调用关系: 在 promise<void> 构造时被框架调用。
  */
 inline auto promise<void>::get_return_object() noexcept -> task<>
 {
     // 从当前 promise 对象创建对应的协程句柄，并用此句柄构造 task 对象
     return task<>{coroutine_handle::from_promise(*this)};
 }
 
 // --- End get_return_object 实现 ---
 
 
 #ifdef DEBUG
 /**
  * @brief 调试辅助函数：从通用协程句柄获取具体类型的 Promise 对象引用
  * @tparam T Promise 的模板参数 (通常是返回值类型，默认为 void)
  * @param handle 通用协程句柄
  * @return 具体类型 Promise 对象的引用
  */
 template<typename T = void>
 inline auto get_promise(std::coroutine_handle<> handle) -> detail::promise<T>& // 修改返回类型
 {
     // 根据 T 是否为 void，选择正确的具体 Promise 类型进行转换
     if constexpr (std::is_void_v<T>) {
         auto specific_handle = std::coroutine_handle<detail::promise<void>>::from_address(handle.address());
         return specific_handle.promise();
     } else {
         auto specific_handle = std::coroutine_handle<detail::promise<T>>::from_address(handle.address());
         return specific_handle.promise();
     }
 }
 #endif // DEBUG
 
 } // namespace detail
 
 } // namespace coro