

#include <gtest/gtest.h>
#include <GAS/utils.hpp>

TEST(ReturnDoubleTest, ReturnsExpectedValue) {
  double val = return_double();
  EXPECT_DOUBLE_EQ(val, 3.14);
}