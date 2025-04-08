#include "coro/engine.hpp"
#include "coro/net/io_info.hpp"
#include "coro/task.hpp"


// 定义一些类型别名，增加可读性 (根据函数签名推断)
using ursptr = io_uring_sqe*; // 指向提交队列条目(SQE)的指针
using urcptr = io_uring_cqe*; // 指向完成队列条目(CQE)的指针

namespace coro::detail
{
using std::memory_order_relaxed; // 使用 relaxed 内存顺序 (通常用于计数器，性能较高)

/**
 * @brief 初始化引擎
 *
 * 在引擎开始运行前进行必要的设置和资源分配。
 * @note 实现逻辑:
 *   1. 设置全局日志信息中的引擎指针 (linfo.egn = this)。
 *   2. 将等待提交的IO计数器 (m_pending_io_submits) 初始化为0。
 *   3. 将正在运行的IO计数器 (m_num_io_running) 初始化为0。
 *   4. 初始化io_uring代理 (m_upxy)，传入队列深度 (config::kEntryLength)。
 *      m_upxy.init() 内部会调用 io_uring_queue_init_params 创建 io_uring 实例和 eventfd。
 */
auto engine::init() noexcept -> void
{
    linfo.egn            = this; // 将当前引擎实例指针赋给日志信息结构体 (假设 linfo 是全局或静态的)
    m_pending_io_submits = 0;    // 初始化等待提交的IO计数为0
    m_num_io_running     = 0;    // 初始化正在运行的IO计数为0
    // 初始化 io_uring 代理，参数为 io_uring 队列的深度 (容量)
    m_upxy.init(config::kEntryLength);
}

/**
 * @brief 反初始化引擎
 *
 * 在引擎停止运行时释放资源，清理状态。
 * @note 实现逻辑:
 *   1. 调用 io_uring 代理的 deinit() 方法，释放 io_uring 相关资源 (包括 eventfd)。
 *   2. 将等待提交和正在运行的 IO 计数器重置为 0。
 *   3. 创建一个空的临时任务队列 `task_queue`。
 *   4. 使用 `swap` 将引擎的任务队列 `m_task_queue` 与空的临时队列交换。
 *      这是一种清空原始队列并释放其可能持有的资源的常用技巧。
 *      (注意：如果队列中的 handle 需要 destroy，这里并没有处理，可能会内存泄漏。
 *       更完善的实现应在 swap 前遍历并 destroy 队列中的 handle。)
 */
auto engine::deinit() noexcept -> void
{
    m_upxy.deinit(); // 反初始化 io_uring 代理，释放资源
    m_pending_io_submits = 0; // 重置等待提交的 IO 计数
    m_num_io_running     = 0; // 重置正在运行的 IO 计数
    // 创建一个空的 MPMC 队列
    mpmc_queue<coroutine_handle<>> task_queue;
    // 将引擎的任务队列与空队列交换，达到清空的目的
    m_task_queue.swap(task_queue);
    // 此时，原 m_task_queue 中的内容（如果有）会被 task_queue 的析构函数处理
    // （假设 mpmc_queue 的析构函数会正确处理）
}

/**
 * @brief 检查引擎是否准备好执行任务
 *
 * 判断引擎当前是否有待处理的任务（在任务队列中）。
 * @return true 如果任务队列不为空，表示有任务待执行。
 * @return false 如果任务队列为空。
 * @note 实现逻辑:
 *   1. 调用任务队列的 `was_empty()` 方法 (假设此方法检查队列是否为空)。
 *   2. 返回其结果的否定 (!)。
 */
auto engine::ready() noexcept -> bool
{
    // 调用任务队列的 was_empty() 方法，如果队列 *不* 为空，则返回 true
    return !m_task_queue.was_empty();
}

/**
 * @brief 获取一个空闲的 io_uring 提交队列条目 (SQE)
 *
 * @return ursptr 指向可用 SQE 的指针，如果 SQ 已满则可能返回 nullptr。
 * @note 实现逻辑:
 *   1. 直接调用 io_uring 代理的 `get_free_sqe()` 方法获取 SQE。
 *   2. **注意:** 这里没有在获取 SQE 后立即调用 `add_io_submit()` 来增加计数。
 *      这意味着增加计数的责任落在了使用这个 SQE 的代码上。
 *      这与之前的代码示例不同，需要注意。
 */
auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes (这部分代码是助教预留的，但当前实现直接调用了代理)
    // 从 io_uring 代理获取一个空闲的 SQE 指针
    return m_upxy.get_free_sqe();
    // 返回获取到的 SQE 指针 (可能是 nullptr)
}

/**
 * @brief 获取当前任务队列中待调度的任务数量
 *
 * @return size_t 队列中的任务数量。
 * @note 实现逻辑:
 *   1. 调用任务队列的 `was_size()` 方法 (假设此方法返回队列当前大小)。
 */
auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes (这部分代码是助教预留的，但当前实现直接调用了队列方法)
    // 返回任务队列的当前大小
    return m_task_queue.was_size();
}

/**
 * @brief 从任务队列中调度（获取并移除）一个协程任务
 *
 * @return coroutine_handle<> 获取到的协程句柄。如果队列为空，行为取决于 `pop()` 的实现
 *         (根据断言，这里期望 `pop()` 在队列非空时总能成功返回一个有效句柄)。
 * @note 实现逻辑:
 *   1. 调用任务队列的 `pop()` 方法，尝试获取并移除队头的一个协程句柄。
 *   2. 使用 `assert(bool(coro))` 断言确保获取到的句柄是有效的 (非空)。
 *      这意味着调用 `schedule()` 的前提是队列必须非空 (通常由 `ready()` 检查)。
 *   3. 返回获取到的协程句柄。
 */
auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes (这部分代码是助教预留的，但当前实现直接调用了队列方法)
    // 从任务队列中弹出一个协程句柄
    auto coro = m_task_queue.pop();
    // 断言确保弹出的句柄有效 (非空)。如果队列为空时调用 pop() 可能触发此断言。
    assert(bool(coro));
    return coro; // 返回弹出的句柄
}

/**
 * @brief 向引擎提交一个新的协程任务
 *
 * 将一个协程句柄放入任务队列等待调度执行。
 * @param handle 要提交的协程句柄。
 * @note 实现逻辑:
 *   1. 使用断言确保提交的句柄非空。
 *   2. 调用任务队列的 `push()` 方法将句柄放入队列。
 *      (注意：如果队列是有界且已满，`push` 行为未知，可能阻塞或失败，这里未处理)。
 *   3. 调用 `wake_up(1)` 方法，向 `eventfd` 写入 1，以唤醒可能在 `poll_submit` 中
 *      因等待 `eventfd` 而阻塞的引擎线程。
 */
auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes (这部分代码是助教预留的，但当前实现已包含核心逻辑)
    // 断言确保提交的句柄非空
    assert(handle != nullptr && "engine get nullptr task handle");
    // 将协程句柄压入任务队列
    m_task_queue.push(handle);
    // 调用 wake_up 通知引擎有新任务（内部会向 eventfd 写入）
    wake_up(task_flag);
}

/**
 * @brief 执行队列中的一个任务
 *
 * 从任务队列中调度一个任务，恢复其执行，并在其完成后进行清理。
 * @note 实现逻辑:
 *   1. 调用 `schedule()` 获取并移除一个协程句柄 `coro`。
 *      (如果队列为空，`schedule` 内的断言会触发)。
 *   2. 调用 `coro.resume()` 恢复协程的执行。
 *   3. 检查协程执行后是否完成 (`coro.done()`)。
 *   4. 如果协程已完成，调用 `clean(coro)` 清理其资源 (通常是销毁协程帧)。
 *      (如果协程未完成，它通常是因为 `co_await` 而暂停，其状态由等待的事件或父协程管理)。
 */
auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule(); // 从队列获取一个任务句柄
    coro.resume();          // 恢复协程执行
    if (coro.done())        // 如果协程执行完毕
    {
        clean(coro);        // 清理协程资源
    }
}

/**
 * @brief 处理一个 io_uring 完成队列条目 (CQE)
 *
 * 根据 CQE 中存储的用户数据 (假设是 io_info 指针) 调用相应的回调函数。
 * @param cqe 指向完成队列条目 (CQE) 的指针。
 * @note 实现逻辑:
 *   1. 使用 `io_uring_cqe_get_data(cqe)` 获取存储在 CQE `user_data` 字段的值。
 *   2. 将获取到的值 (通常是 `uint64_t`) 重新解释为 `net::detail::io_info*` 指针 `data`。
 *      (这要求在提交 SQE 时必须正确地将 `io_info*` 设置到 `user_data`)。
 *   3. 调用 `data` 指针指向的结构体中的回调函数指针 `cb`，并将 `data` 指针自身
 *      和 CQE 的结果 `cqe->res` 作为参数传递给回调函数。
 *      回调函数 `cb` 负责处理具体的 IO 完成逻辑 (例如，恢复等待该 IO 的协程)。
 */
auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    // 从 CQE 的 user_data 中获取关联的 io_info 指针
    auto data = reinterpret_cast<net::detail::io_info*>(io_uring_cqe_get_data(cqe));
    // 调用 io_info 中存储的回调函数 cb，传入 io_info 指针和 IO 结果 (cqe->res)
    data->cb(data, cqe->res);
}

/**
 * @brief 执行一次轮询和提交循环的核心逻辑
 *
 * 负责提交挂起的 IO 请求，等待并处理完成的 IO 事件。
 * 如果没有挂起的 IO 且任务队列为空，则会阻塞等待 eventfd 通知。
 * @note 实现逻辑: (当前版本与问题描述中的修改版本不同，按代码实际逻辑注释)
 *   1. 调用 `do_io_submit()` 尝试提交所有当前挂起的 IO 请求 (`m_pending_io_submits`)。
 *   2. 调用 `m_upxy.wait_eventfd()` 阻塞等待 `eventfd` 被写入。
 *      这个等待可能是因为新任务提交 (`submit_task`->`wake_up`) 或 IO 完成
 *      (如果 `io_uring` 被配置为完成时写入 `eventfd`)。
 *      `wait_eventfd` 返回写入 `eventfd` 的计数值 `cnt`。
 *   3. 调用 `wake_by_cqe(cnt)` (函数未提供，假设它检查 `cnt` 是否表示由 CQE 完成触发)。
 *      如果不是由 CQE 完成触发（例如只是新任务到达），则直接返回，让外部循环处理任务队列。
 *   4. 如果是由 CQE 完成触发 (或 `wake_by_cqe` 返回 true)，则继续执行。
 *   5. 调用 `m_upxy.peek_batch_cqe()` 非阻塞地检查并获取一批已完成的 CQE。
 *      获取的数量上限由当前正在运行的 IO 数量 `m_num_io_running` 决定 (这似乎不太对，
 *      通常应该用 CQ 队列或 `m_urc` 的容量作为上限)。
 *   6. 如果获取到已完成的 CQE (`num > 0`)：
 *      a. 遍历这些 CQE。
 *      b. 对每个 CQE 调用 `handle_cqe_entry()` 处理完成事件（通常是调用回调）。
 *      c. 调用 `m_upxy.cq_advance(num)` 通知 `io_uring` 这些 CQE 已被处理。
 *      d. 原子地减少正在运行的 IO 计数 `m_num_io_running`。
 */
auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes (这部分代码是助教预留的，但已有实现)
    do_io_submit(); // 尝试提交挂起的 IO 请求
    // 等待 eventfd 事件 (可能由新任务或 IO 完成触发)
    auto cnt = m_upxy.wait_eventfd();
    // 检查唤醒是否由 CQE 完成引起 (wake_by_cqe 函数未提供定义)
    if (!wake_by_cqe(cnt))
    {
        // 如果不是 CQE 唤醒 (例如只是新任务)，直接返回让外部循环处理任务队列
        return;
    }
    // 如果是 CQE 唤醒 (或 wake_by_cqe 返回 true)，检查完成队列
    // 获取已完成的 CQE，数量上限由 m_num_io_running 决定 (这里可能需要调整)
    auto num = m_upxy.peek_batch_cqe(m_urc.data(), m_num_io_running.load(std::memory_order_acquire));

    if (num != 0) // 如果获取到已完成的 CQE
    {
        // 处理这一批完成的 IO 事件
        for (int i = 0; i < num; i++)
        {
            handle_cqe_entry(m_urc[i]); // 调用回调处理每个 CQE
        }
        m_upxy.cq_advance(num); // 通知 io_uring 这些 CQE 已处理
        // 原子地减少正在运行的 IO 计数
        m_num_io_running.fetch_sub(num, std::memory_order_acq_rel);
    }
}

/**
 * @brief 原子地增加等待提交的 IO 请求计数并唤醒引擎
 *
 * 当准备好一个新的 IO 操作 (获取了 SQE) 但尚未提交时，调用此函数。
 * 函数会增加等待提交的 IO 计数，并通过 eventfd 唤醒可能在等待的引擎线程。
 * 
 * @note 实现逻辑:
 *   1. 使用 `fetch_add` 原子地将 `m_pending_io_submits` 的值加 1。
 *      使用 `memory_order_relaxed` 是因为这里通常不需要强制的内存同步。
 *   2. 调用 `wake_up(1)` 向 eventfd 写入值 1，唤醒可能在 `poll_submit` 中
 *      等待的线程，使其能够处理新增的 IO 请求。
 *      这确保了 IO 请求能够及时被提交和处理，而不需要等待其他事件触发。
 */
auto engine::add_io_submit() noexcept -> void
{
    // 原子地将等待提交的 IO 计数加 1
    m_pending_io_submits.fetch_add(1, memory_order_relaxed);
    // 唤醒可能在等待的引擎线程，使其能够处理新增的 IO 请求
    wake_up(io_flag);
}

/**
 * @brief 检查引擎当前是否没有待处理或正在进行的 IO 操作
 *
 * @return true 如果等待提交和正在运行的 IO 计数都为 0。
 * @return false 如果有任何等待提交或正在运行的 IO。
 * @note 实现逻辑:
 *   1. 原子地读取等待提交的 IO 计数 `m_pending_io_submits`。
 *   2. 原子地读取正在运行的 IO 计数 `m_num_io_running`。
 *      使用 `memory_order_acquire` 确保能看到其他线程对这些计数的最新修改。
 *   3. 当且仅当两个计数都为 0 时，返回 true。
 */
auto engine::empty_io() noexcept -> bool
{
    // 检查等待提交的 IO 计数是否为 0
    return m_pending_io_submits.load(std::memory_order_acquire) == 0 &&
           // 并且检查正在运行的 IO 计数是否也为 0
           m_num_io_running.load(std::memory_order_acquire) == 0;
}

/**
 * @brief 唤醒可能在等待 eventfd 的引擎线程
 *
 * 向与引擎关联的 eventfd 写入一个值，解除 `wait_eventfd` 的阻塞。
 * 该函数使用预定义的标志位系统来区分不同类型的事件：
 * - task_flag (1<<44): 用于标识任务事件，表示有新任务被提交
 * - io_flag (1<<24): 用于标识IO事件，表示有IO操作需要提交
 * - cqe_mask 区域: 用于标识IO完成事件
 * 
 * 当引擎线程从 eventfd 读取值时，可以通过 wake_by_task()、wake_by_io() 
 * 和 wake_by_cqe() 宏来判断是哪种类型的事件。
 *
 * @param val 要写入 eventfd 的值，通常是预定义的标志位之一或它们的组合。
 * @note 实现逻辑:
 *   1. 调用 io_uring 代理的 `write_eventfd()` 方法，将 `val` 写入 eventfd。
 *   2. 这会触发任何在 `wait_eventfd()` 上阻塞的线程继续执行。
 */
auto engine::wake_up(uint64_t val) noexcept -> void
{
    m_upxy.write_eventfd(val); // 通过 io_uring 代理向 eventfd 写入值
}

/**
 * @brief 执行实际的 IO 提交操作
 *
 * 检查是否有等待提交的 IO 请求，如果有，则调用 io_uring 代理进行提交。
 * @note 实现逻辑:
 *   1. 原子地读取等待提交的 IO 数量 `num_task_wait` (使用 acquire 确保看到最新值)。
 *   2. 如果 `num_task_wait > 0`：
 *      a. 调用 `m_upxy.submit()` 提交 IO 请求，获取实际提交的数量 `num`。
 *      b. 从期望提交数 `num_task_wait` 中减去实际提交数 `num`。
 *      c. 使用断言确保所有等待的请求都被提交了 (`num_task_wait == 0`)。
 *         (这个断言可能过于严格，如果 `submit` 可能只提交部分请求，这里会失败)。
 *      d. 原子地增加正在运行的 IO 计数 `m_num_io_running` (增加 `num`)。
 *         使用 acq_rel 确保内存操作的顺序。
 *      e. 原子地减少等待提交的 IO 计数 `m_pending_io_submits` (减少 `num`)。
 *         使用 acq_rel 确保内存操作的顺序。
 */
auto engine::do_io_submit() noexcept -> void
{
    // 读取等待提交的 IO 数量
    int num_task_wait = m_pending_io_submits.load(std::memory_order_acquire);
    if (num_task_wait > 0) // 如果有等待提交的 IO
    {
        int num = m_upxy.submit(); // 调用代理提交 IO，获取实际提交的数量
        num_task_wait -= num;      // 更新还需等待的数量
        // 断言检查是否所有等待的 IO 都被提交了
        assert(num_task_wait == 0);
        // 增加正在运行的 IO 计数 (原子操作)
        m_num_io_running.fetch_add(num, std::memory_order_acq_rel);
        // 减少等待提交的 IO 计数 (原子操作)
        // 必须在增加 m_num_io_running 之后执行 (注释说明)
        m_pending_io_submits.fetch_sub(num, std::memory_order_acq_rel);
    }
}
}; // namespace coro::detail