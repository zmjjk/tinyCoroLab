#include "coro/engine.hpp"
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

class EngineTest : public ::testing::Test
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine m_engine;
};

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_F(EngineTest, InitStateCase1)
{
    EXPECT_FALSE(m_engine.ready());
    EXPECT_TRUE(m_engine.empty_io());
    EXPECT_EQ(m_engine.num_task_schedule(), 0);
    EXPECT_EQ(m_engine.get_id(), detail::local_engine().get_id());
}
