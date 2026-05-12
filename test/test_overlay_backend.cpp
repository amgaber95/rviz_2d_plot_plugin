// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include "overlay_backend.hpp"

using rviz_2d_plot_plugin::OverlayGeometry;
using rviz_2d_plot_plugin::OverlayHorizontalAlignment;
using rviz_2d_plot_plugin::OverlayVerticalAlignment;
using rviz_2d_plot_plugin::alignedOverlayPosition;
using rviz_2d_plot_plugin::clampedOverlaySize;

TEST(OverlayBackend, ClampsTextureDimensionsToAtLeastOnePixel)
{
  const auto clamped = clampedOverlaySize(-40, 0);

  EXPECT_EQ(clamped.width, 1U);
  EXPECT_EQ(clamped.height, 1U);
}

TEST(OverlayBackend, KeepsPositiveTextureDimensions)
{
  const auto clamped = clampedOverlaySize(420, 180);

  EXPECT_EQ(clamped.width, 420U);
  EXPECT_EQ(clamped.height, 180U);
}

TEST(OverlayBackend, ComputesLeftTopAlignedPosition)
{
  OverlayGeometry geometry;
  geometry.width = 360;
  geometry.height = 220;
  geometry.x_offset = 12;
  geometry.y_offset = 18;
  geometry.horizontal_alignment = OverlayHorizontalAlignment::Left;
  geometry.vertical_alignment = OverlayVerticalAlignment::Top;

  const auto position = alignedOverlayPosition(geometry);

  EXPECT_DOUBLE_EQ(position.left, 12.0);
  EXPECT_DOUBLE_EQ(position.top, 18.0);
}

TEST(OverlayBackend, ComputesCenteredAlignedPosition)
{
  OverlayGeometry geometry;
  geometry.width = 360;
  geometry.height = 220;
  geometry.x_offset = 12;
  geometry.y_offset = 18;
  geometry.horizontal_alignment = OverlayHorizontalAlignment::Center;
  geometry.vertical_alignment = OverlayVerticalAlignment::Center;

  const auto position = alignedOverlayPosition(geometry);

  EXPECT_DOUBLE_EQ(position.left, -168.0);
  EXPECT_DOUBLE_EQ(position.top, -92.0);
}

TEST(OverlayBackend, ComputesRightBottomAlignedPosition)
{
  OverlayGeometry geometry;
  geometry.width = 360;
  geometry.height = 220;
  geometry.x_offset = 12;
  geometry.y_offset = 18;
  geometry.horizontal_alignment = OverlayHorizontalAlignment::Right;
  geometry.vertical_alignment = OverlayVerticalAlignment::Bottom;

  const auto position = alignedOverlayPosition(geometry);

  EXPECT_DOUBLE_EQ(position.left, -372.0);
  EXPECT_DOUBLE_EQ(position.top, -238.0);
}
