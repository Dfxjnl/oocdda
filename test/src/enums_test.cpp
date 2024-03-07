#include <stdexcept>

#include <gtest/gtest.h>

#include "enums.h"

TEST(FlagToBitPositionTest, HandlesZero) { EXPECT_EQ(flag_to_bit_position(0), 1); }

TEST(FlagToBitPositionTest, HandlesPositiveNumbers)
{
    EXPECT_EQ(flag_to_bit_position(1), 2);
    EXPECT_EQ(flag_to_bit_position(2), 4);
    EXPECT_EQ(flag_to_bit_position(3), 8);
}

TEST(FlagToBitPositionTest, HandlesNegativeNumbers)
{
    EXPECT_THROW(flag_to_bit_position(-1), std::out_of_range);
}
