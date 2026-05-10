// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include "rviz_2d_plot_plugin/plot_value_formatter.hpp"

using rviz_2d_plot_plugin::formatAxisTickValue;
using rviz_2d_plot_plugin::formatPlotValue;

TEST(PlotValueFormatter, FormatsLegendValuesWithoutUnneededDecimals)
{
  EXPECT_EQ(formatPlotValue(30.0), "30");
  EXPECT_EQ(formatPlotValue(3.0), "3");
  EXPECT_EQ(formatPlotValue(2.5), "2.5");
  EXPECT_EQ(formatPlotValue(0.14), "0.14");
  EXPECT_EQ(formatPlotValue(-0.03327), "-0.03327");
}

TEST(PlotValueFormatter, FormatsAxisTicksFromTickStep)
{
  EXPECT_EQ(formatAxisTickValue(30.0, 10.0), "30");
  EXPECT_EQ(formatAxisTickValue(2.5, 0.5), "2.5");
  EXPECT_EQ(formatAxisTickValue(0.05, 0.01), "0.05");
  EXPECT_EQ(formatAxisTickValue(0.1000000000001, 0.05), "0.1");
  EXPECT_EQ(formatAxisTickValue(1e-12, 0.5), "0");
}
