#include "gtest/gtest.h"

int add(int x, int y)
{
  return x + y;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(example, sum_zero)
{
  auto result = add(5, -5);
  ASSERT_EQ(result, 0);
}

TEST(example, sum_five)
{
  auto result = add(5, 10);
  ASSERT_EQ(result, 15);
}
