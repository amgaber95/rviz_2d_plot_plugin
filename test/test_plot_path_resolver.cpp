// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "rviz_2d_plot_plugin/plot_path_resolver.hpp"

using rviz_2d_plot_plugin::PlotPathStatus;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::resolvePlotPath;
using rviz_2d_plot_plugin::resolveTopicFieldPath;
using rviz_2d_plot_plugin::splitFieldPath;

TEST(PlotPathResolver, SplitsFieldPaths)
{
  const std::vector<std::string> expected{"twist", "twist", "linear", "x"};

  EXPECT_EQ(splitFieldPath("twist/twist/linear/x"), expected);
  EXPECT_EQ(splitFieldPath("/twist//twist/linear/x/"), expected);
}

TEST(PlotPathResolver, ResolvesLongestTopicPrefix)
{
  TopicTypeMap topics{
    {"/odom", {"nav_msgs/msg/Odometry"}},
    {"/odom/twist", {"geometry_msgs/msg/Twist"}}};

  const auto result = resolvePlotPath("/odom/twist/twist/linear/x", topics);

  ASSERT_EQ(result.status, PlotPathStatus::Ok);
  EXPECT_EQ(result.topic, "/odom/twist");
  EXPECT_EQ(result.type, "geometry_msgs/msg/Twist");
  const std::vector<std::string> expected{"twist", "linear", "x"};
  EXPECT_EQ(result.field_segments, expected);
}

TEST(PlotPathResolver, ResolvesSeparateTopicAndField)
{
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};

  const auto result = resolveTopicFieldPath("/cmd_vel", "linear/x", topics);

  ASSERT_EQ(result.status, PlotPathStatus::Ok);
  EXPECT_EQ(result.topic, "/cmd_vel");
  EXPECT_EQ(result.type, "geometry_msgs/msg/Twist");
  const std::vector<std::string> expected{"linear", "x"};
  EXPECT_EQ(result.field_segments, expected);
}

TEST(PlotPathResolver, RejectsUnsupportedPaths)
{
  TopicTypeMap topics{{"/scan", {"sensor_msgs/msg/LaserScan"}}};

  EXPECT_EQ(resolvePlotPath("", topics).status, PlotPathStatus::EmptyPath);
  EXPECT_EQ(resolvePlotPath("scan/ranges", topics).status, PlotPathStatus::InvalidPath);
  EXPECT_EQ(resolvePlotPath("/scan/ranges[]", topics).status, PlotPathStatus::UnsupportedSyntax);
  EXPECT_EQ(resolvePlotPath("/scan/ranges[abc]", topics).status, PlotPathStatus::UnsupportedSyntax);
  EXPECT_EQ(resolvePlotPath("/scan/ranges[0]", topics).status, PlotPathStatus::Ok);
}

TEST(PlotPathResolver, ReportsTopicAndFieldErrors)
{
  TopicTypeMap topics{
    {"/cmd_vel", {"geometry_msgs/msg/Twist"}},
    {"/value", {"std_msgs/msg/Float64", "std_msgs/msg/Int32"}}};

  EXPECT_EQ(resolvePlotPath("/odom/pose/x", topics).status, PlotPathStatus::WaitingForTopic);
  EXPECT_EQ(resolvePlotPath("/cmd_vel", topics).status, PlotPathStatus::MissingFieldPath);
  EXPECT_EQ(resolvePlotPath("/value/data", topics).status, PlotPathStatus::AmbiguousTopicType);
  EXPECT_EQ(resolveTopicFieldPath("/cmd_vel", "", topics).status, PlotPathStatus::MissingFieldPath);
}
