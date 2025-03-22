#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <tuple>
#include <vector>

#include "coro/coro.hpp"
#include "gtest/gtest.h"

using namespace coro;

/*************************************************************
 *                       pre-definition                      *
 *************************************************************/

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class ChannelTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int              id;
        channel<int>     m_ch;
        std::vector<int> m_vec;
        mutex            m_mtx;
    };

protected:
    test_paras m_para;
};

task<> producer(ChannelTest::test_paras& para, int id, const int num_per_producer)
{
    for (int i = 0; i < num_per_producer; i++)
    {
        co_await para.m_ch.send(id * num_per_producer + i);
    }
    para.m_ch.close();
}

task<> consumer(ChannelTest::test_paras& para)
{
    std::vector<int> vec;
    while (true)
    {
        auto data = co_await para.m_ch.recv();
        if (data)
        {
            vec.push_back(*data);
        }
        else
        {
            break;
        }
    }

    para.m_mtx.lock();
    para.m_vec.insert(para.m_vec.end(), vec.begin(), vec.end());
    para.m_mtx.unlock();
}
/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(ChannelTest, ChannelProducerConsumer)
{
    int producer_num, consumer_num, num_per_producer;
    std::tie(producer_num, consumer_num, num_per_producer) = GetParam();

    const int task_num = num_per_producer * producer_num;

    scheduler::init();

    for (int i = 0; i < producer_num; i++)
    {
        submit_to_scheduler(producer(m_para, i, num_per_producer));
    }

    for (int i = 0; i < consumer_num; i++)
    {
        submit_to_scheduler(consumer(m_para));
    }

    scheduler::start();
    scheduler::loop(false);

    ASSERT_EQ(m_para.m_vec.size(), task_num);
    std::sort(m_para.m_vec.begin(), m_para.m_vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_para.m_vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ChannelTests,
    ChannelTest,
    ::testing::Values(
        std::tuple(1, 1, 100),
        std::tuple(1, 1, 1000),
        std::tuple(1, 1, 10000),
        std::tuple(1, 1, 100000),

        std::tuple(10, 10, 100),
        std::tuple(10, 10, 10000),

        std::tuple(100, 100, 100),
        std::tuple(100, 100, 1000),

        std::tuple(1000, 1000, 1),
        std::tuple(1000, 1000, 100),

        std::tuple(1, 1000, 100000),
        std::tuple(1000, 1, 100)));
