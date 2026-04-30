// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include "rviz_2d_plot_plugin/tick_generator.hpp"

using rviz_2d_plot_plugin::generateTicks;
using rviz_2d_plot_plugin::niceTickStep;
using rviz_2d_plot_plugin::PlotRange;

TEST(TickGenerator, ChoosesRoundedTickSteps)
{
  EXPECT_DOUBLE_EQ(niceTickStep(0.03), 0.05);
  EXPECT_DOUBLE_EQ(niceTickStep(0.8), 1.0);
  EXPECT_DOUBLE_EQ(niceTickStep(1.1), 2.0);
  EXPECT_DOUBLE_EQ(niceTickStep(3.0), 5.0);
  EXPECT_DOUBLE_EQ(niceTickStep(8.0), 10.0);
}

TEST(TickGenerator, GeneratesMajorTicksInsideRange)
{
  const auto ticks = generateTicks(PlotRange{0.0, 10.0}, 6);

  ASSERT_EQ(ticks.major.size(), 6u);
  EXPECT_DOUBLE_EQ(ticks.major[0], 0.0);
  EXPECT_DOUBLE_EQ(ticks.major[1], 2.0);
  EXPECT_DOUBLE_EQ(ticks.major[2], 4.0);
  EXPECT_DOUBLE_EQ(ticks.major[3], 6.0);
  EXPECT_DOUBLE_EQ(ticks.major[4], 8.0);
  EXPECT_DOUBLE_EQ(ticks.major[5], 10.0);
}

TEST(TickGenerator, HandlesNegativeAndPositiveRanges)
{
  const auto ticks = generateTicks(PlotRange{-1.2, 1.2}, 6);

  ASSERT_EQ(ticks.major.size(), 5u);
  EXPECT_DOUBLE_EQ(ticks.major[0], -1.0);
  EXPECT_DOUBLE_EQ(ticks.major[1], -0.5);
  EXPECT_DOUBLE_EQ(ticks.major[2], 0.0);
  EXPECT_DOUBLE_EQ(ticks.major[3], 0.5);
  EXPECT_DOUBLE_EQ(ticks.major[4], 1.0);
}

TEST(TickGenerator, HandlesTimeRanges)
{
  const auto ticks = generateTicks(PlotRange{-50.0, 0.0}, 6);

  ASSERT_EQ(ticks.major.size(), 6u);
  EXPECT_DOUBLE_EQ(ticks.major[0], -50.0);
  EXPECT_DOUBLE_EQ(ticks.major[1], -40.0);
  EXPECT_DOUBLE_EQ(ticks.major[2], -30.0);
  EXPECT_DOUBLE_EQ(ticks.major[3], -20.0);
  EXPECT_DOUBLE_EQ(ticks.major[4], -10.0);
  EXPECT_DOUBLE_EQ(ticks.major[5], 0.0);
}

TEST(TickGenerator, GeneratesMinorTicksBetweenMajorTicks)
{
  const auto ticks = generateTicks(PlotRange{0.0, 4.0}, 3, 2);

  ASSERT_EQ(ticks.major.size(), 3u);
  ASSERT_EQ(ticks.minor.size(), 4u);
  EXPECT_NEAR(ticks.minor[0], 2.0 / 3.0, 1e-9);
  EXPECT_NEAR(ticks.minor[1], 4.0 / 3.0, 1e-9);
  EXPECT_NEAR(ticks.minor[2], 8.0 / 3.0, 1e-9);
  EXPECT_NEAR(ticks.minor[3], 10.0 / 3.0, 1e-9);
}

TEST(TickGenerator, RepairsInvalidRanges)
{
  const auto ticks = generateTicks(PlotRange{3.0, 3.0}, 5);

  ASSERT_FALSE(ticks.major.empty());
  EXPECT_LE(ticks.major.front(), 3.0);
  EXPECT_GE(ticks.major.back(), 3.0);
}
