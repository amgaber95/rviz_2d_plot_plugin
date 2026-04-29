// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include "rviz_2d_plot_plugin/sample_buffer.hpp"

using rviz_2d_plot_plugin::RollingSampleBuffer;

TEST(RollingSampleBuffer, AppendsSamplesAndReportsLatest)
{
  RollingSampleBuffer buffer;

  EXPECT_TRUE(buffer.empty());

  buffer.append(1.0, 10.0);
  buffer.append(2.0, 12.5);

  ASSERT_EQ(buffer.size(), 2u);
  EXPECT_FALSE(buffer.empty());
  EXPECT_DOUBLE_EQ(buffer.samples()[0].time, 1.0);
  EXPECT_DOUBLE_EQ(buffer.samples()[0].value, 10.0);
  EXPECT_DOUBLE_EQ(buffer.samples()[1].time, 2.0);
  EXPECT_DOUBLE_EQ(buffer.samples()[1].value, 12.5);

  ASSERT_TRUE(buffer.latest().has_value());
  EXPECT_DOUBLE_EQ(buffer.latest()->time, 2.0);
  EXPECT_DOUBLE_EQ(buffer.latest()->value, 12.5);
}

TEST(RollingSampleBuffer, PrunesSamplesOlderThanCutoff)
{
  RollingSampleBuffer buffer;
  buffer.append(1.0, 10.0);
  buffer.append(2.0, 20.0);
  buffer.append(3.0, 30.0);

  buffer.pruneBefore(2.0);

  ASSERT_EQ(buffer.size(), 2u);
  EXPECT_DOUBLE_EQ(buffer.samples()[0].time, 2.0);
  EXPECT_DOUBLE_EQ(buffer.samples()[1].time, 3.0);
}

TEST(RollingSampleBuffer, PrunesToRollingWindow)
{
  RollingSampleBuffer buffer;
  buffer.append(1.0, 10.0);
  buffer.append(3.0, 30.0);
  buffer.append(5.0, 50.0);

  buffer.pruneToWindow(5.0, 2.0);

  ASSERT_EQ(buffer.size(), 2u);
  EXPECT_DOUBLE_EQ(buffer.samples()[0].time, 3.0);
  EXPECT_DOUBLE_EQ(buffer.samples()[1].time, 5.0);
}

TEST(RollingSampleBuffer, ComputesValueRangeAndClearsSamples)
{
  RollingSampleBuffer buffer;
  EXPECT_FALSE(buffer.valueRange().has_value());

  buffer.append(1.0, 4.0);
  buffer.append(2.0, -2.0);
  buffer.append(3.0, 7.5);

  ASSERT_TRUE(buffer.valueRange().has_value());
  EXPECT_DOUBLE_EQ(buffer.valueRange()->min, -2.0);
  EXPECT_DOUBLE_EQ(buffer.valueRange()->max, 7.5);

  buffer.clear();

  EXPECT_TRUE(buffer.empty());
  EXPECT_FALSE(buffer.latest().has_value());
  EXPECT_FALSE(buffer.valueRange().has_value());
}
