// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "plot_2d_display_test_helpers.hpp"

namespace
{

using rviz_2d_plot_plugin::AxisScaleMode;
using rviz_2d_plot_plugin::HorizontalAlignment;
using rviz_2d_plot_plugin::LegendPosition;
using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::PlotMode;
using rviz_2d_plot_plugin::Plot2DDisplay;
using rviz_2d_plot_plugin::Plot2DDisplayTestAccessor;
using rviz_2d_plot_plugin::QoSDurability;
using rviz_2d_plot_plugin::QoSReliability;
using rviz_2d_plot_plugin::TimeSource;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::VerticalAlignment;
using rviz_2d_plot_plugin::XAxisMode;
using rviz_2d_plot_plugin::XYAxisScaleMode;
using rviz_2d_plot_plugin::XYHistoryMode;
using rviz_2d_plot_plugin::test::RecordingOverlayBackend;
using rviz_2d_plot_plugin::test::childNames;
using rviz_2d_plot_plugin::test::completionsFor;
using rviz_2d_plot_plugin::test::ensureQtApplication;
using rviz_2d_plot_plugin::test::findChild;
using rviz_2d_plot_plugin::test::functionBody;
using rviz_2d_plot_plugin::test::plotDisplaySource;
using rviz_2d_plot_plugin::test::processQtEvents;
using rviz_2d_plot_plugin::test::serializeMessage;

}  // namespace

TEST(Plot2DDisplay, TopicOptionsListVisibleTopicsSortedByName)
{
  ensureQtApplication();
  Plot2DDisplay display;
  Plot2DDisplayTestAccessor::setTopics(
    display,
    TopicTypeMap{
    {"/zed/odom", {"nav_msgs/msg/Odometry"}},
    {"/cmd_vel_out", {"geometry_msgs/msg/Twist"}},
    {"/diagnostics", {"std_msgs/msg/Float64"}},
  });

  const std::vector<std::string> options =
    Plot2DDisplayTestAccessor::topicOptions(display);

  const std::vector<std::string> expected{
    "/cmd_vel_out",
    "/diagnostics",
    "/zed/odom",
  };
  EXPECT_EQ(options, expected);
}

TEST(Plot2DDisplay, TopicGraphFailuresDoNotEscapeDisplayQueries)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopicProvider(
    display,
    []() -> TopicTypeMap {
      throw std::runtime_error("context is invalid");
    });

  EXPECT_NO_THROW(Plot2DDisplayTestAccessor::resolveAndSubscribe(display));
  EXPECT_TRUE(Plot2DDisplayTestAccessor::topicOptions(display).empty());
  EXPECT_TRUE(Plot2DDisplayTestAccessor::fieldOptions(display, "/value").empty());
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).status,
    PlotControllerStatus::WaitingForTopic);
}

TEST(Plot2DDisplay, FieldOptionsListNumericScalarsForSelectedTopic)
{
  ensureQtApplication();
  Plot2DDisplay display;
  Plot2DDisplayTestAccessor::setTopics(
    display,
    TopicTypeMap{{"/cmd_vel_out", {"geometry_msgs/msg/Twist"}}});

  const std::vector<std::string> options =
    Plot2DDisplayTestAccessor::fieldOptions(display, "/cmd_vel_out");

  EXPECT_NE(std::find(options.begin(), options.end(), "linear/x"), options.end());
  EXPECT_NE(std::find(options.begin(), options.end(), "angular/z"), options.end());
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::fieldOptions(display, "/missing").size(), 0U);
}
