#include <algorithm>
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

using ::coro::net::detail::cb_type;
using ::coro::net::detail::io_info;
using ::coro::net::detail::io_type;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EngineTest : public ::testing::Test
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine   m_engine;
    std::vector<int> m_vec;
};

class EngineNopIOTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine       m_engine;
    std::vector<int>     m_vec;
    std::vector<io_info> m_infos;
};

class EngineMultiThreadNopIOTest : public EngineNopIOTest
{
};

task<> func1(std::vector<int>& vec, int val)
{
    vec.push_back(val);
    co_return;
}

void io_cb(io_info* info, int res)
{
    auto num = reinterpret_cast<int*>(info->data);
    *num     = res;
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_F(EngineTest, InitStateCase1)
{
    ASSERT_FALSE(m_engine.ready());
    ASSERT_TRUE(m_engine.empty_io());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_engine.get_id(), detail::local_engine().get_id());
}

TEST_F(EngineTest, ExecOneDetachTaskByUser)
{
    auto task = func1(m_vec, 1);
    m_engine.submit_task(task.handle());
    task.detach();

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);
    auto handle = m_engine.schedule();
    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    handle.resume();
    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
    handle.destroy();
}

TEST_F(EngineTest, ExecMultiDetachTaskByUser)
{
    const int task_num = 100;
    for (int i = 0; i < task_num; i++)
    {
        auto task = func1(m_vec, i);
        m_engine.submit_task(task.handle());
        task.detach();
    }

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), task_num);
    while (m_engine.ready())
    {
        auto handle = m_engine.schedule();
        handle.resume();
        handle.destroy();
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_vec.size(), task_num);
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

TEST_F(EngineTest, ExecOneDetachTaskByEngine)
{
    auto task = func1(m_vec, 1);
    m_engine.submit_task(task.handle());
    task.detach();

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);

    m_engine.exec_one_task();

    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);

    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
}

TEST_F(EngineTest, ExecMultiDetachTaskByEngine)
{
    const int task_num = 100;
    for (int i = 0; i < task_num; i++)
    {
        auto task = func1(m_vec, i);
        m_engine.submit_task(task.handle());
        task.detach();
    }

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), task_num);
    while (m_engine.ready())
    {
        m_engine.exec_one_task();
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_vec.size(), task_num);
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

TEST_F(EngineTest, ExecOneNoDetachTaskByEngine)
{
    auto task = func1(m_vec, 1);
    m_engine.submit_task(task.handle());

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);
    m_engine.exec_one_task();
    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);

    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
}

TEST_F(EngineTest, AddIOSubmit)
{
    m_engine.add_io_submit();
    ASSERT_FALSE(m_engine.empty_io());
}

TEST_P(EngineNopIOTest, AddNopIO)
{
    int task_num = GetParam();
    m_infos.resize(task_num);
    m_vec = std::vector<int>(task_num, 1);
    for (int i = 0; i < task_num; i++)
    {
        m_infos[i].data = reinterpret_cast<uintptr_t>(&m_vec[i]);
        m_infos[i].cb   = io_cb;
        auto sqe        = m_engine.get_free_urs();
        ASSERT_NE(sqe, nullptr);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, &m_infos[i]);
        m_engine.add_io_submit();
    }

    ASSERT_FALSE(m_engine.empty_io());

    m_engine.poll_submit();

    ASSERT_TRUE(m_engine.empty_io());

    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], 0);
    }
}

INSTANTIATE_TEST_SUITE_P(EngineNopIOTests, EngineNopIOTest, ::testing::Values(1, 100, 10000));

TEST_F(EngineTest, LoopAddNopIO)
{
    const int loop_num = 2 * config::kEntryLength;
    m_vec.push_back(0);
    for (int i = 0; i < loop_num; i++)
    {
        m_vec[0] = 1;
        io_info info;
        info.data = reinterpret_cast<uintptr_t>(&m_vec[0]);
        info.cb   = io_cb;

        auto sqe = m_engine.get_free_urs();
        ASSERT_NE(sqe, nullptr);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, &info);

        m_engine.add_io_submit();
        ASSERT_FALSE(m_engine.empty_io());
        m_engine.poll_submit();
        ASSERT_TRUE(m_engine.empty_io());
        ASSERT_EQ(m_vec[0], 0);
    }
}

TEST_F(EngineTest, SubmitTaskToEngine)
{
    m_vec.push_back(0);
    auto task = func1(m_vec, 0);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::sleep(2);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    ASSERT_EQ(m_vec[0], 1);
}

TEST_F(EngineTest, SubmitTaskToEngineExecByUser)
{
    m_vec.push_back(0);
    auto task = func1(m_vec, 2);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::sleep(2);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    task.handle().resume();

    ASSERT_EQ(m_vec.size(), 2);
    ASSERT_EQ(m_vec[0], 1);
    ASSERT_EQ(m_vec[1], 2);
}

TEST_F(EngineTest, SubmitTaskToEngineExecByEngine)
{
    m_vec.push_back(0);
    auto task = func1(m_vec, 2);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
            m_engine.exec_one_task();
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::sleep(1);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    ASSERT_EQ(m_vec.size(), 2);
    ASSERT_EQ(m_vec[0], 1);
    ASSERT_EQ(m_vec[1], 2);
}

TEST_P(EngineMultiThreadNopIOTest, ThreadAddNopIO)
{
    int thread_num = GetParam();

    auto t = std::thread(
        [&]()
        {
            int count = 0;
            while (count < thread_num)
            {
                m_engine.poll_submit();
                while (m_engine.ready())
                {
                    ++count;
                    m_engine.exec_one_task();
                }
            }
        });

    std::vector<std::thread> vec;
    for (int i = 0; i < thread_num; i++)
    {
        vec.push_back(std::thread(
            [&]()
            {
                auto task = func1(m_vec, i);
                m_engine.submit_task(task.handle());
                task.detach();
            }));
    }

    t.join();
    for (int i = 0; i < thread_num; i++)
    {
        vec[i].join();
    }

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < thread_num; i++)
    {
        EXPECT_EQ(m_vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(EngineMultiThreadNopIOTests, EngineMultiThreadNopIOTest, ::testing::Values(1, 10, 100));
