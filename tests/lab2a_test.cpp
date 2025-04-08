#include <algorithm>
#include <thread>
#include <tuple>
#include <vector>

#include "coro/engine.hpp"
#include "coro/net/io_info.hpp"
#include "coro/task.hpp"
#include "coro/utils.hpp"
#include "gtest/gtest.h"

using namespace coro;

/*************************************************************
 *                       pre-definition                      *
 *************************************************************/

// 使用命名空间中的类型别名
using ::coro::net::detail::cb_type;
using ::coro::net::detail::io_info;
using ::coro::net::detail::io_type;

// 主函数，初始化Google Test并运行所有测试
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// 基本引擎测试类，用于测试引擎的基本功能
class EngineTest : public ::testing::Test
{
protected:
    // 测试前初始化引擎
    void SetUp() override { m_engine.init(); }

    // 测试后清理引擎资源
    void TearDown() override { m_engine.deinit(); }

    detail::engine   m_engine;    // 测试用的引擎实例
    std::vector<int> m_vec;       // 用于验证测试结果的向量
};

// 引擎NOP IO测试类，用于测试引擎处理无操作IO的能力
class EngineNopIOTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine       m_engine;
    std::vector<int>     m_vec;
    std::vector<io_info> m_infos;  // 存储IO信息的向量
};

// 引擎多线程任务测试类，继承自EngineNopIOTest
class EngineMultiThreadTaskTest : public EngineNopIOTest
{
};

// 引擎混合任务和NOP IO测试类，用于测试同时处理任务和IO的能力
class EngineMixTaskNopIOTest : public ::testing::TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine       m_engine;
    std::vector<int>     m_vec;
    std::vector<io_info> m_infos;
};

// 测试用协程函数，将值添加到向量中
task<> func(std::vector<int>& vec, int val)
{
    vec.push_back(val);
    co_return;
}

// IO回调函数，用于处理IO完成事件
void io_cb(io_info* info, int res)
{
    auto num = reinterpret_cast<int*>(info->data);
    *num     = res;
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

// 测试引擎初始状态是否正确
TEST_F(EngineTest, InitStateCase)
{
    ASSERT_FALSE(m_engine.ready());                                     // 引擎应该没有待处理的任务
    ASSERT_TRUE(m_engine.empty_io());                                   // 引擎应该没有待处理的IO
    ASSERT_EQ(m_engine.num_task_schedule(), 0);                         // 任务队列应该为空
    ASSERT_EQ(m_engine.get_id(), detail::local_engine().get_id());      // 引擎ID应该与本地引擎ID一致
}

// 测试提交分离任务但由用户执行
TEST_F(EngineTest, ExecOneDetachTaskByUser)
{
    auto task = func(m_vec, 1);                // 创建一个协程任务
    m_engine.submit_task(task.handle());       // 提交任务到引擎
    task.detach();                             // 分离任务（放弃所有权）

    ASSERT_TRUE(m_engine.ready());             // 引擎应该有待处理的任务
    ASSERT_EQ(m_engine.num_task_schedule(), 1);// 任务队列应该有一个任务
    auto handle = m_engine.schedule();         // 获取任务句柄
    ASSERT_FALSE(m_engine.ready());            // 引擎应该没有待处理的任务了
    ASSERT_EQ(m_engine.num_task_schedule(), 0);// 任务队列应该为空
    handle.resume();                           // 恢复协程执行
    ASSERT_EQ(m_vec.size(), 1);                // 向量应该有一个元素
    ASSERT_EQ(m_vec[0], 1);                    // 元素值应该为1
    handle.destroy();                          // 销毁协程句柄
}

// 测试提交多个分离任务但由用户执行
TEST_F(EngineTest, ExecMultiDetachTaskByUser)
{
    const int task_num = 100;                  // 任务数量
    for (int i = 0; i < task_num; i++)
    {
        auto task = func(m_vec, i);            // 创建协程任务
        m_engine.submit_task(task.handle());   // 提交任务到引擎
        task.detach();                         // 分离任务
    }

    ASSERT_TRUE(m_engine.ready());             // 引擎应该有待处理的任务
    ASSERT_EQ(m_engine.num_task_schedule(), task_num); // 任务队列应该有task_num个任务
    while (m_engine.ready())
    {
        auto handle = m_engine.schedule();     // 获取任务句柄
        handle.resume();                       // 恢复协程执行
        handle.destroy();                      // 销毁协程句柄
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);// 任务队列应该为空
    ASSERT_EQ(m_vec.size(), task_num);         // 向量应该有task_num个元素

    std::sort(m_vec.begin(), m_vec.end());     // 排序向量
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);                // 验证元素值
    }
}

// 测试提交分离任务但由引擎执行，引擎应该销毁任务句柄，否则会内存泄漏
TEST_F(EngineTest, ExecOneDetachTaskByEngine)
{
    auto task = func(m_vec, 1);                // 创建协程任务
    m_engine.submit_task(task.handle());       // 提交任务到引擎
    task.detach();                             // 分离任务

    ASSERT_TRUE(m_engine.ready());             // 引擎应该有待处理的任务
    ASSERT_EQ(m_engine.num_task_schedule(), 1);// 任务队列应该有一个任务

    m_engine.exec_one_task();                  // 引擎执行一个任务

    ASSERT_FALSE(m_engine.ready());            // 引擎应该没有待处理的任务了
    ASSERT_EQ(m_engine.num_task_schedule(), 0);// 任务队列应该为空

    ASSERT_EQ(m_vec.size(), 1);                // 向量应该有一个元素
    ASSERT_EQ(m_vec[0], 1);                    // 元素值应该为1
}

// 测试提交多个分离任务但由引擎执行，引擎应该销毁任务句柄，否则会内存泄漏
TEST_F(EngineTest, ExecMultiDetachTaskByEngine)
{
    const int task_num = 100;                  // 任务数量
    for (int i = 0; i < task_num; i++)
    {
        auto task   = func(m_vec, i);          // 创建协程任务
        auto handle = task.handle();           // 获取任务句柄
        task.detach();                         // 分离任务
        m_engine.submit_task(handle);          // 提交任务到引擎
    }

    ASSERT_TRUE(m_engine.ready());             // 引擎应该有待处理的任务
    ASSERT_EQ(m_engine.num_task_schedule(), task_num); // 任务队列应该有task_num个任务
    while (m_engine.ready())
    {
        m_engine.exec_one_task();              // 引擎执行一个任务
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);// 任务队列应该为空
    ASSERT_EQ(m_vec.size(), task_num);         // 向量应该有task_num个元素

    std::sort(m_vec.begin(), m_vec.end());     // 排序向量
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);                // 验证元素值
    }
}

// 测试提交任务但由引擎执行，引擎不应该销毁任务句柄，否则会导致指针二次释放
TEST_F(EngineTest, ExecOneNoDetachTaskByEngine)
{
    auto task = func(m_vec, 1);                // 创建协程任务
    m_engine.submit_task(task.handle());       // 提交任务到引擎

    ASSERT_TRUE(m_engine.ready());             // 引擎应该有待处理的任务
    ASSERT_EQ(m_engine.num_task_schedule(), 1);// 任务队列应该有一个任务
    m_engine.exec_one_task();                  // 引擎执行一个任务
    ASSERT_FALSE(m_engine.ready());            // 引擎应该没有待处理的任务了
    ASSERT_EQ(m_engine.num_task_schedule(), 0);// 任务队列应该为空

    ASSERT_EQ(m_vec.size(), 1);                // 向量应该有一个元素
    ASSERT_EQ(m_vec[0], 1);                    // 元素值应该为1
}

// 测试在引擎轮询前添加无操作IO
TEST_F(EngineTest, AddNopIOBeforePoll)
{
    io_info info;                              // IO信息
    m_vec.push_back(1);                        // 向量添加元素1

    auto io_thread = std::thread(
        [&]()
        {
            info.data = reinterpret_cast<uintptr_t>(&m_vec[0]); // 设置IO数据指针
            info.cb   = io_cb;                                  // 设置IO回调函数
            auto sqe  = m_engine.get_free_urs();                // 获取空闲的SQE
            ASSERT_NE(sqe, nullptr);                            // SQE不应该为空指针
            io_uring_prep_nop(sqe);                             // 准备无操作IO
            io_uring_sqe_set_data(sqe, &info);                  // 设置SQE的用户数据
            m_engine.add_io_submit();                           // 通知引擎有IO需要提交
        });

    auto poll_thread = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保IO线程先完成
            do
            {
                m_engine.poll_submit();        // 引擎轮询提交IO并处理完成事件
            } while (!m_engine.empty_io());    // 直到没有待处理的IO
        });

    io_thread.join();                          // 等待IO线程完成
    poll_thread.join();                        // 等待轮询线程完成

    ASSERT_EQ(m_vec[0], 0);                    // 元素值应该被回调函数修改为0
}

// 测试在引擎轮询后添加无操作IO
TEST_F(EngineTest, AddNopIOAfterPoll)
{
    io_info info;                              // IO信息
    m_vec.push_back(1);                        // 向量添加元素1

    auto io_thread = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保轮询线程先开始
            info.data = reinterpret_cast<uintptr_t>(&m_vec[0]); // 设置IO数据指针
            info.cb   = io_cb;                                  // 设置IO回调函数
            auto sqe  = m_engine.get_free_urs();                // 获取空闲的SQE
            ASSERT_NE(sqe, nullptr);                            // SQE不应该为空
            io_uring_prep_nop(sqe);                             // 准备无操作IO
            io_uring_sqe_set_data(sqe, &info);                  // 设置SQE的用户数据
            m_engine.add_io_submit();                           // 通知引擎有IO需要提交
        });

    auto poll_thread = std::thread(
        [&]()
        {
            do
            {
                m_engine.poll_submit();        // 引擎轮询提交IO并处理完成事件
            } while (!m_engine.empty_io());    // 直到没有待处理的IO
        });

    io_thread.join();                          // 等待IO线程完成
    poll_thread.join();                        // 等待轮询线程完成

    ASSERT_EQ(m_vec[0], 0);                    // 元素值应该被回调函数修改为0
}

// 测试批量添加无操作IO
TEST_P(EngineNopIOTest, AddBatchNopIO)
{
    int task_num = GetParam();                 // 获取参数化测试的参数
    m_infos.resize(task_num);                  // 调整IO信息向量大小
    m_vec = std::vector<int>(task_num, 1);     // 创建指定大小的向量，初始值为1
    for (int i = 0; i < task_num; i++)
    {
        m_infos[i].data = reinterpret_cast<uintptr_t>(&m_vec[i]); // 设置IO数据指针
        m_infos[i].cb   = io_cb;                                  // 设置IO回调函数
        auto sqe        = m_engine.get_free_urs();                // 获取空闲的SQE
        ASSERT_NE(sqe, nullptr);                                  // SQE不应该为空
        io_uring_prep_nop(sqe);                                   // 准备无操作IO
        io_uring_sqe_set_data(sqe, &m_infos[i]);                  // 设置SQE的用户数据
        m_engine.add_io_submit();                                 // 通知引擎有IO需要提交
    }

    do
    {
        m_engine.poll_submit();                // 引擎轮询提交IO并处理完成事件
    } while (!m_engine.empty_io());            // 直到没有待处理的IO

    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], 0);                // 所有元素值应该被回调函数修改为0
    }
}

// 实例化参数化测试，测试不同数量的无操作IO
INSTANTIATE_TEST_SUITE_P(EngineNopIOTests, EngineNopIOTest, ::testing::Values(1, 100, 10000));

// 测试循环添加无操作IO
TEST_F(EngineTest, LoopAddNopIO)
{
    const int loop_num = 2 * config::kEntryLength; // 循环次数为队列容量的两倍
    m_vec.push_back(0);                        // 向量添加元素0
    for (int i = 0; i < loop_num; i++)
    {
        m_vec[0] = 1;                          // 设置元素值为1
        io_info info;                          // IO信息
        info.data = reinterpret_cast<uintptr_t>(&m_vec[0]); // 设置IO数据指针
        info.cb   = io_cb;                                  // 设置IO回调函数

        auto sqe = m_engine.get_free_urs();    // 获取空闲的SQE
        ASSERT_NE(sqe, nullptr);               // SQE不应该为空
        io_uring_prep_nop(sqe);                // 准备无操作IO
        io_uring_sqe_set_data(sqe, &info);     // 设置SQE的用户数据
        m_engine.add_io_submit();              // 通知引擎有IO需要提交

        do
        {
            m_engine.poll_submit();            // 引擎轮询提交IO并处理完成事件
        } while (!m_engine.empty_io());        // 直到没有待处理的IO
        ASSERT_EQ(m_vec[0], 0);                // 元素值应该被回调函数修改为0
    }
}

// 测试在引擎轮询后提交任务
TEST_F(EngineTest, LastSubmitTaskToEngine)
{
    m_vec.push_back(0);                        // 向量添加元素0
    auto task = func(m_vec, 0);                // 创建协程任务

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();            // 引擎轮询提交IO并处理完成事件
            m_vec[0] = 1;                      // 设置元素值为1
            ASSERT_TRUE(m_engine.ready());     // 引擎应该有待处理的任务
            ASSERT_EQ(m_engine.num_task_schedule(), 1); // 任务队列应该有一个任务
            ASSERT_TRUE(m_engine.empty_io());  // 引擎应该没有待处理的IO
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保t1先开始
            m_vec[0] = 2;                      // 设置元素值为2
            m_engine.submit_task(task.handle()); // 提交任务到引擎
        });
    t1.join();                                 // 等待t1完成
    t2.join();                                 // 等待t2完成

    ASSERT_EQ(m_vec[0], 1);                    // 元素值应该为1（t1设置的值）
}

// 测试在引擎轮询前提交任务
TEST_F(EngineTest, FirstSubmitTaskToEngine)
{
    m_vec.push_back(0);                        // 向量添加元素0
    auto task = func(m_vec, 0);                // 创建协程任务

    auto t1 = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保t2先完成
            m_engine.poll_submit();            // 引擎轮询提交IO并处理完成事件
            m_vec[0] = 1;                      // 设置元素值为1
            ASSERT_TRUE(m_engine.ready());     // 引擎应该有待处理的任务
            ASSERT_EQ(m_engine.num_task_schedule(), 1); // 任务队列应该有一个任务
            ASSERT_TRUE(m_engine.empty_io());  // 引擎应该没有待处理的IO
        });
    auto t2 = std::thread(
        [&]()
        {
            m_vec[0] = 2;                      // 设置元素值为2
            m_engine.submit_task(task.handle()); // 提交任务到引擎
        });
    t1.join();                                 // 等待t1完成
    t2.join();                                 // 等待t2完成

    ASSERT_EQ(m_vec[0], 1);                    // 元素值应该为1（t1设置的值）
}

// 测试提交任务但由用户执行
TEST_F(EngineTest, SubmitTaskToEngineExecByUser)
{
    m_vec.push_back(0);                        // 向量添加元素0
    auto task = func(m_vec, 2);                // 创建协程任务

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();            // 引擎轮询提交IO并处理完成事件
            m_vec[0] = 1;                      // 设置元素值为1
            ASSERT_TRUE(m_engine.ready());     // 引擎应该有待处理的任务
            ASSERT_EQ(m_engine.num_task_schedule(), 1); // 任务队列应该有一个任务
            ASSERT_TRUE(m_engine.empty_io());  // 引擎应该没有待处理的IO
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保t1先开始
            m_vec[0] = 2;                      // 设置元素值为2
            m_engine.submit_task(task.handle()); // 提交任务到引擎
        });
    t1.join();                                 // 等待t1完成
    t2.join();                                 // 等待t2完成

    task.handle().resume();                    // 用户手动恢复协程执行

    ASSERT_EQ(m_vec.size(), 2);                // 向量应该有两个元素
    ASSERT_EQ(m_vec[0], 1);                    // 第一个元素值应该为1（t1设置的值）
    ASSERT_EQ(m_vec[1], 2);                    // 第二个元素值应该为2（任务添加的值）
}

// 测试提交任务但由引擎执行
TEST_F(EngineTest, SubmitTaskToEngineExecByEngine)
{
    m_vec.push_back(0);                        // 向量添加元素0
    auto task = func(m_vec, 2);                // 创建协程任务

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();            // 引擎轮询提交IO并处理完成事件
            m_vec[0] = 1;                      // 设置元素值为1
            ASSERT_TRUE(m_engine.ready());     // 引擎应该有待处理的任务
            ASSERT_EQ(m_engine.num_task_schedule(), 1); // 任务队列应该有一个任务
            ASSERT_TRUE(m_engine.empty_io());  // 引擎应该没有待处理的IO
            m_engine.exec_one_task();          // 引擎执行一个任务
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);                // 休眠100毫秒，确保t1先开始
            m_vec[0] = 2;                      // 设置元素值为2
            m_engine.submit_task(task.handle()); // 提交任务到引擎
        });
    t1.join();                                 // 等待t1完成
    t2.join();                                 // 等待t2完成

    ASSERT_EQ(m_vec.size(), 2);                // 向量应该有两个元素
    ASSERT_EQ(m_vec[0], 1);                    // 第一个元素值应该为1（t1设置的值）
    ASSERT_EQ(m_vec[1], 2);                    // 第二个元素值应该为2（任务添加的值）
}

// 测试多线程提交任务
TEST_P(EngineMultiThreadTaskTest, MultiThreadAddTask)
{
    int thread_num = GetParam();               // 获取参数化测试的参数（线程数量）

    auto t = std::thread(
        [&]()
        {
            int count = 0;
            while (count < thread_num)
            {
                m_engine.poll_submit();        // 引擎轮询提交IO并处理完成事件
                while (m_engine.ready())
                {
                    ++count;                   // 计数器增加
                    m_engine.exec_one_task();  // 引擎执行一个任务
                }
            }
        });

    std::vector<std::thread> vec;              // 线程向量
    for (int i = 0; i < thread_num; i++)
    {
        vec.push_back(std::thread(
            [&, i]()
            {
                auto task   = func(m_vec, i);  // 创建协程任务
                auto handle = task.handle();   // 获取任务句柄
                task.detach();                 // 分离任务
                m_engine.submit_task(handle);  // 提交任务到引擎
            }));
    }

    t.join();                                  // 等待主线程完成
    for (int i = 0; i < thread_num; i++)
    {
        vec[i].join();                         // 等待所有工作线程完成
    }

    std::sort(m_vec.begin(), m_vec.end());     // 排序向量
    for (int i = 0; i < thread_num; i++)
    {
        EXPECT_EQ(m_vec[i], i);                // 验证元素值
    }
}

// 实例化参数化测试，测试不同数量的线程
INSTANTIATE_TEST_SUITE_P(EngineMultiThreadTaskTests, EngineMultiThreadTaskTest, ::testing::Values(1, 10, 100));

// 测试同时提交任务和无操作IO
TEST_P(EngineMixTaskNopIOTest, MixTaskNopIO)
{
    int task_num, nopio_num;
    std::tie(task_num, nopio_num) = GetParam(); // 获取参数化测试的参数（任务数量和IO数量）
    m_vec.resize(nopio_num);                    // 调整向量大小
    m_infos.resize(nopio_num);                  // 调整IO信息向量大小

    auto task_thread = std::thread(
        [&]()
        {
            for (int i = 0; i < task_num; i++)
            {
                auto task   = func(m_vec, nopio_num + i); // 创建协程任务
                auto handle = task.handle();              // 获取任务句柄
                task.detach();                            // 分离任务
                m_engine.submit_task(handle);             // 提交任务到引擎
                if ((i + 1) % 100 == 0)
                {
                    utils::msleep(10);                    // 每提交100个任务休眠10毫秒
                }
            }
        });

    auto io_thread = std::thread(
        [&]()
        {
            for (int i = 0; i < nopio_num; i++)
            {
                m_infos[i].data = reinterpret_cast<uintptr_t>(&m_vec[i]); // 设置IO数据指针
                m_infos[i].cb   = io_cb;                                  // 设置IO回调函数

                auto sqe = m_engine.get_free_urs();                       // 获取空闲的SQE
                ASSERT_NE(sqe, nullptr);                                  // SQE不应该为空
                io_uring_prep_nop(sqe);                                   // 准备无操作IO
                io_uring_sqe_set_data(sqe, &m_infos[i]);                  // 设置SQE的用户数据

                m_engine.add_io_submit();                                 // 通知引擎有IO需要提交
                if ((i + 1) % 100 == 0)
                {
                    utils::msleep(10);                                    // 每提交100个IO休眠10毫秒
                }
            }

            // 这将使轮询线程在IO线程之后完成
            auto task   = func(m_vec, task_num + nopio_num);              // 创建额外的协程任务
            auto handle = task.handle();                                  // 获取任务句柄
            task.detach();                                                // 分离任务
            m_engine.submit_task(handle);                                 // 提交任务到引擎
        });

    auto poll_thread = std::thread(
        [&]()
        {
            int cnt = 0;
            while (cnt < task_num + 1)
            {
                m_engine.poll_submit();        // 引擎轮询提交IO并处理完成事件
                while (m_engine.ready())
                {
                    m_engine.exec_one_task();  // 引擎执行一个任务
                    cnt++;                     // 计数器增加
                }
            }
        });

    task_thread.join();                        // 等待任务线程完成
    io_thread.join();                          // 等待IO线程完成
    poll_thread.join();                        // 等待轮询线程完成

    ASSERT_EQ(m_vec.size(), task_num + nopio_num + 1); // 向量大小应该等于任务数+IO数+1
    std::sort(m_vec.begin(), m_vec.end());     // 排序向量
    for (int i = 0; i < nopio_num; i++)
    {
        ASSERT_EQ(m_vec[i], 0);                // IO操作完成后，前nopio_num个元素应该为0
    }
    for (int i = 0; i < task_num + 1; i++)
    {
        ASSERT_EQ(m_vec[i + nopio_num], i + nopio_num); // 后task_num+1个元素应该为对应索引值
    }
}

// 实例化参数化测试，测试不同数量的任务和IO组合
INSTANTIATE_TEST_SUITE_P(
    EngineMixTaskNopIOTests,
    EngineMixTaskNopIOTest,
    ::testing::Values(
        std::make_tuple(1, 1),                 // 1个任务，1个IO
        std::make_tuple(100, 100),             // 100个任务，100个IO
        std::make_tuple(1000, 1000),           // 1000个任务，1000个IO
        std::make_tuple(10000, 10000)));       // 10000个任务，10000个IO
