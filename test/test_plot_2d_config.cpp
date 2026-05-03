// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <limits>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"

using rviz_2d_plot_plugin::AxisConfig;
using rviz_2d_plot_plugin::AxisScaleMode;
using rviz_2d_plot_plugin::LineStyle;
using rviz_2d_plot_plugin::PlotStyle;
using rviz_2d_plot_plugin::Plot2DConfig;

TEST(Plot2DConfig, DefaultsDescribeOneUsableTimeSeries)
{
  const Plot2DConfig config;

  ASSERT_EQ(config.series.size(), 1u);
  EXPECT_TRUE(config.series.front().enabled);
  EXPECT_EQ(config.series.front().topic, "");
  EXPECT_EQ(config.series.front().field, "");
  EXPECT_EQ(config.series.front().label, "Series");
  EXPECT_EQ(config.series.front().color.red, 80);
  EXPECT_EQ(config.series.front().color.green, 170);
  EXPECT_EQ(config.series.front().color.blue, 255);
  EXPECT_DOUBLE_EQ(config.series.front().line_width, 2.0);
  EXPECT_DOUBLE_EQ(config.series.front().line_alpha, 1.0);
  EXPECT_EQ(config.series.front().line_style, LineStyle::Solid);
  EXPECT_EQ(config.series.front().plot_style, PlotStyle::Line);
  EXPECT_DOUBLE_EQ(config.series.front().value_scale, 1.0);
  EXPECT_DOUBLE_EQ(config.series.front().value_offset, 0.0);

  EXPECT_EQ(config.y_axis.scale_mode, AxisScaleMode::Auto);
  EXPECT_DOUBLE_EQ(config.y_axis.fixed_min, -1.0);
  EXPECT_DOUBLE_EQ(config.y_axis.fixed_max, 1.0);
  EXPECT_DOUBLE_EQ(config.y_axis.padding_fraction, 0.08);

  EXPECT_DOUBLE_EQ(config.time.window_seconds, 30.0);
  EXPECT_DOUBLE_EQ(config.time.refresh_rate_hz, 20.0);
  EXPECT_FALSE(config.time.paused);

  EXPECT_EQ(config.layout.width, 360);
  EXPECT_EQ(config.layout.height, 220);
  EXPECT_EQ(config.layout.x_offset, 10);
  EXPECT_EQ(config.layout.y_offset, 10);
}

TEST(Plot2DConfig, RepairsInvalidFixedAxisRange)
{
  AxisConfig axis;
  axis.scale_mode = AxisScaleMode::Fixed;
  axis.fixed_min = 5.0;
  axis.fixed_max = 5.0;

  axis.repairFixedRange();

  EXPECT_LT(axis.fixed_min, axis.fixed_max);
  EXPECT_DOUBLE_EQ(axis.fixed_min, 4.5);
  EXPECT_DOUBLE_EQ(axis.fixed_max, 5.5);
}

TEST(Plot2DConfig, RepairsInvalidTimeAndLayoutValues)
{
  Plot2DConfig config;
  config.time.window_seconds = -3.0;
  config.time.refresh_rate_hz = 0.0;
  config.layout.width = 40;
  config.layout.height = 20;

  config.repair();

  EXPECT_DOUBLE_EQ(config.time.window_seconds, 30.0);
  EXPECT_DOUBLE_EQ(config.time.refresh_rate_hz, 20.0);
  EXPECT_EQ(config.layout.width, 120);
  EXPECT_EQ(config.layout.height, 80);
}

TEST(Plot2DConfig, RepairsInvalidSeriesAppearanceValues)
{
  Plot2DConfig config;
  config.series.front().color.red = -4;
  config.series.front().color.green = 300;
  config.series.front().color.blue = 120;
  config.series.front().line_width = -2.0;
  config.series.front().line_alpha = 2.0;
  config.series.front().value_scale = std::numeric_limits<double>::infinity();
  config.series.front().value_offset = std::numeric_limits<double>::quiet_NaN();

  config.repair();

  EXPECT_EQ(config.series.front().color.red, 0);
  EXPECT_EQ(config.series.front().color.green, 255);
  EXPECT_EQ(config.series.front().color.blue, 120);
  EXPECT_DOUBLE_EQ(config.series.front().line_width, 1.0);
  EXPECT_DOUBLE_EQ(config.series.front().line_alpha, 1.0);
  EXPECT_DOUBLE_EQ(config.series.front().value_scale, 1.0);
  EXPECT_DOUBLE_EQ(config.series.front().value_offset, 0.0);
}
