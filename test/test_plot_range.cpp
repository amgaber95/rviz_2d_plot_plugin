// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <vector>

#include "rviz_2d_plot_plugin/plot_range.hpp"
#include "rviz_2d_plot_plugin/sample_buffer.hpp"

using rviz_2d_plot_plugin::makeAutoRange;
using rviz_2d_plot_plugin::makeFixedRange;
using rviz_2d_plot_plugin::PlotSample;

TEST(PlotRange, KeepsValidFixedRanges)
{
  const auto range = makeFixedRange(-2.0, 8.0);

  EXPECT_DOUBLE_EQ(range.min, -2.0);
  EXPECT_DOUBLE_EQ(range.max, 8.0);
  EXPECT_DOUBLE_EQ(range.span(), 10.0);
}

TEST(PlotRange, RepairsInvalidFixedRanges)
{
  const auto range = makeFixedRange(5.0, 5.0);

  EXPECT_DOUBLE_EQ(range.min, 4.5);
  EXPECT_DOUBLE_EQ(range.max, 5.5);
}

TEST(PlotRange, EmptyAutoRangeUsesReadableDefault)
{
  const std::vector<PlotSample> samples;
  const auto range = makeAutoRange(samples, 0.08);

  EXPECT_DOUBLE_EQ(range.min, -1.0);
  EXPECT_DOUBLE_EQ(range.max, 1.0);
}

TEST(PlotRange, FlatAutoRangeExpandsAroundValue)
{
  const std::vector<PlotSample> samples{{1.0, 5.0}, {2.0, 5.0}};
  const auto range = makeAutoRange(samples, 0.08);

  EXPECT_DOUBLE_EQ(range.min, 4.5);
  EXPECT_DOUBLE_EQ(range.max, 5.5);
}

TEST(PlotRange, AutoRangePadsNonFlatValues)
{
  const std::vector<PlotSample> samples{{1.0, -2.0}, {2.0, 8.0}};
  const auto range = makeAutoRange(samples, 0.10);

  EXPECT_DOUBLE_EQ(range.min, -3.0);
  EXPECT_DOUBLE_EQ(range.max, 9.0);
}

TEST(PlotRange, BoolLikeValuesHaveReadablePadding)
{
  const std::vector<PlotSample> samples{{1.0, 0.0}, {2.0, 1.0}};
  const auto range = makeAutoRange(samples, 0.10);

  EXPECT_DOUBLE_EQ(range.min, -0.1);
  EXPECT_DOUBLE_EQ(range.max, 1.1);
}
