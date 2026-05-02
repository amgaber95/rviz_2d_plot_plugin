// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"
#include "rviz_2d_plot_plugin/plot_path_resolver.hpp"

using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::Plot2DController;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::TopicTypeMap;

namespace
{

rclcpp::SerializedMessage serializeTwist(
  const double linear_x,
  const double angular_z = 0.0)
{
  geometry_msgs::msg::Twist message;
  message.linear.x = linear_x;
  message.angular.z = angular_z;

  rclcpp::Serialization<geometry_msgs::msg::Twist> serializer;
  rclcpp::SerializedMessage serialized;
  serializer.serialize_message(&message, &serialized);
  return serialized;
}

Plot2DConfig makeConfig()
{
  Plot2DConfig config;
  config.series[0].topic = "/cmd_vel";
  config.series[0].field = "linear/x";
  config.time.window_seconds = 1.0;
  return config;
}

}  // namespace

TEST(Plot2DController, ResolvesConfiguredSeries)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};

  controller.configure(makeConfig(), topics);

  ASSERT_EQ(controller.state().status, PlotControllerStatus::Ok);
  ASSERT_EQ(controller.state().series.size(), 1U);
  EXPECT_EQ(controller.state().series[0].topic, "/cmd_vel");
  EXPECT_EQ(controller.state().series[0].type, "geometry_msgs/msg/Twist");
  EXPECT_TRUE(controller.state().series[0].samples.empty());
}

TEST(Plot2DController, AppendsExtractedSamplesAndPrunesToWindow)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  controller.configure(makeConfig(), topics);

  EXPECT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(1.5), 10.0));
  EXPECT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(2.5), 12.0));

  ASSERT_EQ(controller.state().status, PlotControllerStatus::Ok);
  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().time, 12.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 2.5);
  ASSERT_TRUE(controller.state().series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(controller.state().series[0].latest_value.value(), 2.5);
}

TEST(Plot2DController, AppendsSamplesForMultipleConfiguredSeries)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.series.push_back(config.series.front());
  config.series[0].field = "linear/x";
  config.series[0].label = "Linear X";
  config.series[1].field = "angular/z";
  config.series[1].label = "Angular Z";
  controller.configure(config, topics);

  EXPECT_TRUE(
    controller.appendSerializedMessage(
      "/cmd_vel", serializeTwist(1.5, -0.4), 10.0));

  ASSERT_EQ(controller.state().series.size(), 2U);
  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  ASSERT_EQ(controller.state().series[1].samples.size(), 1U);
  EXPECT_EQ(controller.state().series[0].label, "Linear X");
  EXPECT_EQ(controller.state().series[1].label, "Angular Z");
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 1.5);
  EXPECT_DOUBLE_EQ(controller.state().series[1].samples.samples().front().value, -0.4);
}

TEST(Plot2DController, DisabledSeriesDoesNotHideValidSeriesStatus)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.series.push_back(config.series.front());
  config.series[0].enabled = false;
  config.series[1].field = "angular/z";
  controller.configure(config, topics);

  EXPECT_EQ(controller.state().status, PlotControllerStatus::Ok);
  EXPECT_TRUE(
    controller.appendSerializedMessage(
      "/cmd_vel", serializeTwist(1.5, -0.4), 10.0));

  ASSERT_EQ(controller.state().series.size(), 2U);
  EXPECT_TRUE(controller.state().series[0].samples.empty());
  ASSERT_EQ(controller.state().series[1].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[1].samples.samples().front().value, -0.4);
}

TEST(Plot2DController, IgnoresUnrelatedTopicsAndPausedState)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.time.paused = true;
  controller.configure(config, topics);

  EXPECT_FALSE(controller.appendSerializedMessage("/other", serializeTwist(1.5), 10.0));
  EXPECT_FALSE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(2.5), 10.0));

  EXPECT_TRUE(controller.state().series[0].samples.empty());
}

TEST(Plot2DController, ReportsResolutionErrorsWithoutAppending)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.series[0].topic = "/odom";

  controller.configure(config, topics);

  EXPECT_EQ(controller.state().status, PlotControllerStatus::WaitingForTopic);
  EXPECT_FALSE(controller.appendSerializedMessage("/odom", serializeTwist(1.5), 10.0));
  ASSERT_EQ(controller.state().series.size(), 1U);
  EXPECT_TRUE(controller.state().series[0].samples.empty());
}

TEST(Plot2DController, ClearsHistoryWithoutReconfiguring)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  controller.configure(makeConfig(), topics);
  ASSERT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(1.5), 10.0));

  controller.clearHistory();

  EXPECT_TRUE(controller.state().series[0].samples.empty());
  EXPECT_FALSE(controller.state().series[0].latest_value.has_value());
  EXPECT_EQ(controller.state().status, PlotControllerStatus::Ok);
}
