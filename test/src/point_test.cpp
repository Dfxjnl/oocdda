#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "point.hpp"

using oocdda::find_adjacent_points;
using oocdda::Point;

TEST(FindAdjacentPointsTest, NegativeRange)
{
    constexpr Point center {2, 3};
    const auto result {find_adjacent_points(center, -1)};

    ASSERT_TRUE(result.empty());
}

TEST(FindAdjacentPointsTest, RangeZero)
{
    constexpr Point center {2, 3};
    const auto result {find_adjacent_points(center, 0)};

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].x, 2);
    EXPECT_EQ(result[0].y, 3);
}

TEST(FindAdjacentPointsTest, RangeOne)
{
    constexpr Point center {2, 3};
    const auto result {find_adjacent_points(center, 1)};

    ASSERT_EQ(result.size(), 9);

    const std::vector<Point> expected {
        {1, 2},
        {1, 3},
        {1, 4},
        {2, 2},
        {2, 3},
        {2, 4},
        {3, 2},
        {3, 3},
        {3, 4}
    };

    EXPECT_THAT(result, ::testing::UnorderedElementsAreArray(expected));
}
