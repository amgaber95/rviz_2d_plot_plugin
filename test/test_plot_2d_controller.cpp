// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"
#include "rviz_2d_plot_plugin/plot_path_resolver.hpp"

using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::Plot2DController;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::PlotMode;
using rviz_2d_plot_plugin::TimeSource;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::XYHistoryMode;

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

rclcpp::SerializedMessage serializePoseStamped(
  const double position_x,
  const int32_t stamp_sec,
  const uint32_t stamp_nanosec)
{
  geometry_msgs::msg::PoseStamped message;
  message.header.stamp.sec = stamp_sec;
  message.header.stamp.nanosec = stamp_nanosec;
  message.pose.position.x = position_x;

  rclcpp::Serialization<geometry_msgs::msg::PoseStamped> serializer;
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

TEST(Plot2DController, RecoversFromTransientExtractionErrorOnNextValidMessage)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  controller.configure(makeConfig(), topics);

  rclcpp::SerializedMessage malformed;
  EXPECT_FALSE(controller.appendSerializedMessage("/cmd_vel", malformed, 10.0));
  ASSERT_EQ(controller.state().series[0].status, PlotControllerStatus::ExtractionError);

  EXPECT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(2.5), 11.0));

  EXPECT_EQ(controller.state().series[0].status, PlotControllerStatus::Ok);
  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().time, 11.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 2.5);
}

TEST(Plot2DController, AppliesSeriesScaleAndOffsetToExtractedSamples)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.series[0].value_scale = 3.0;
  config.series[0].value_offset = -1.0;
  controller.configure(config, topics);

  EXPECT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(2.0), 10.0));

  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 5.0);
  ASSERT_TRUE(controller.state().series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(controller.state().series[0].latest_value.value(), 5.0);
}

TEST(Plot2DController, UsesMessageHeaderStampWhenConfigured)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/pose", {"geometry_msgs/msg/PoseStamped"}}};
  Plot2DConfig config;
  config.series[0].topic = "/pose";
  config.series[0].field = "pose/position/x";
  config.time.source = TimeSource::HeaderStamp;
  config.time.window_seconds = 10.0;
  controller.configure(config, topics);

  EXPECT_TRUE(
    controller.appendSerializedMessage(
      "/pose", serializePoseStamped(3.5, 12, 250000000), 99.0));

  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().time, 12.25);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 3.5);
}

TEST(Plot2DController, ExtractsSameTopicXAndYFieldsInXYMode)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/pose", {"geometry_msgs/msg/PoseStamped"}}};
  Plot2DConfig config;
  config.plot_mode = PlotMode::XY;
  config.series[0].topic = "/pose";
  config.series[0].x_field = "pose/position/x";
  config.series[0].y_field = "pose/position/y";
  config.time.window_seconds = 10.0;
  controller.configure(config, topics);

  geometry_msgs::msg::PoseStamped message;
  message.pose.position.x = 2.5;
  message.pose.position.y = -1.25;
  rclcpp::Serialization<geometry_msgs::msg::PoseStamped> serializer;
  rclcpp::SerializedMessage serialized;
  serializer.serialize_message(&message, &serialized);

  EXPECT_TRUE(controller.appendSerializedMessage("/pose", serialized, 20.0));

  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().time, 20.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().x, 2.5);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, -1.25);
}

TEST(Plot2DController, KeepsAllSamplesForXYAllSamplesHistoryMode)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/pose", {"geometry_msgs/msg/PoseStamped"}}};
  Plot2DConfig config;
  config.plot_mode = PlotMode::XY;
  config.time.xy_history_mode = XYHistoryMode::AllSamples;
  config.time.window_seconds = 1.0;
  config.series[0].topic = "/pose";
  config.series[0].x_field = "pose/position/x";
  config.series[0].y_field = "pose/position/y";
  controller.configure(config, topics);

  geometry_msgs::msg::PoseStamped message;
  rclcpp::Serialization<geometry_msgs::msg::PoseStamped> serializer;
  rclcpp::SerializedMessage serialized;

  message.pose.position.x = 1.0;
  message.pose.position.y = 2.0;
  serializer.serialize_message(&message, &serialized);
  EXPECT_TRUE(controller.appendSerializedMessage("/pose", serialized, 10.0));

  serialized = rclcpp::SerializedMessage{};
  message.pose.position.x = 3.0;
  message.pose.position.y = 4.0;
  serializer.serialize_message(&message, &serialized);
  EXPECT_TRUE(controller.appendSerializedMessage("/pose", serialized, 20.0));

  ASSERT_EQ(controller.state().series[0].samples.size(), 2U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().x, 1.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 2.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().back().x, 3.0);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().back().value, 4.0);
}

TEST(Plot2DController, FallsBackToReceiveTimeWhenHeaderStampIsUnavailable)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.time.source = TimeSource::HeaderStamp;
  controller.configure(config, topics);

  EXPECT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(1.5), 42.0));

  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().time, 42.0);
}

TEST(Plot2DController, ReconfigureRewritesPreservedSamplesForTransformChange)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  config.series[0].value_scale = 2.0;
  config.series[0].value_offset = 1.0;
  controller.configure(config, topics);
  ASSERT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(2.0), 10.0));
  ASSERT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 5.0);

  config.series[0].value_scale = 3.0;
  config.series[0].value_offset = -2.0;
  controller.configure(config, topics);

  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 4.0);
  ASSERT_TRUE(controller.state().series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(controller.state().series[0].latest_value.value(), 4.0);
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

TEST(Plot2DController, ReconfigurePreservesSamplesForUnchangedSeriesSource)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  controller.configure(config, topics);
  ASSERT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(1.5), 10.0));

  config.series[0].label = "Styled Linear X";
  config.series[0].line_width = 4.0;
  controller.configure(config, topics);

  ASSERT_EQ(controller.state().series.size(), 1U);
  EXPECT_EQ(controller.state().series[0].label, "Styled Linear X");
  ASSERT_EQ(controller.state().series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(controller.state().series[0].samples.samples().front().value, 1.5);
  ASSERT_TRUE(controller.state().series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(controller.state().series[0].latest_value.value(), 1.5);
}

TEST(Plot2DController, ReconfigureDropsSamplesWhenSeriesSourceChanges)
{
  Plot2DController controller;
  TopicTypeMap topics{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}};
  Plot2DConfig config = makeConfig();
  controller.configure(config, topics);
  ASSERT_TRUE(controller.appendSerializedMessage("/cmd_vel", serializeTwist(1.5), 10.0));

  config.series[0].field = "angular/z";
  controller.configure(config, topics);

  ASSERT_EQ(controller.state().series.size(), 1U);
  EXPECT_TRUE(controller.state().series[0].samples.empty());
  EXPECT_FALSE(controller.state().series[0].latest_value.has_value());
}
