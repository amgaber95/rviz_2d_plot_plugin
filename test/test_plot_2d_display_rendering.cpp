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

TEST(Plot2DDisplay, InitializesInjectedOverlayBackend)
{
  ensureQtApplication();
  Plot2DDisplay display;
  RecordingOverlayBackend * backend = nullptr;
  Plot2DDisplayTestAccessor::setOverlayBackendFactory(
    display,
    [&backend](std::string) {
      auto created = std::make_unique<RecordingOverlayBackend>();
      backend = created.get();
      return created;
    });

  Plot2DDisplayTestAccessor::initializeOverlayBackend(display);

  ASSERT_NE(nullptr, backend);
  EXPECT_EQ(backend->initialize_calls, 1);
  ASSERT_FALSE(backend->geometries.empty());
  EXPECT_EQ(backend->geometries.back().width, 360);
  EXPECT_EQ(backend->geometries.back().height, 220);
  EXPECT_EQ(backend->geometries.back().x_offset, 10);
  EXPECT_EQ(backend->geometries.back().y_offset, 10);
  ASSERT_FALSE(backend->visibility.empty());
  EXPECT_FALSE(backend->visibility.back());
}

TEST(Plot2DDisplay, EnableAndDisableToggleOverlayBackendVisibility)
{
  ensureQtApplication();
  Plot2DDisplay display;
  RecordingOverlayBackend * backend = nullptr;
  Plot2DDisplayTestAccessor::setOverlayBackendFactory(
    display,
    [&backend](std::string) {
      auto created = std::make_unique<RecordingOverlayBackend>();
      backend = created.get();
      return created;
    });
  Plot2DDisplayTestAccessor::initializeOverlayBackend(display);
  ASSERT_NE(nullptr, backend);
  backend->visibility.clear();

  Plot2DDisplayTestAccessor::enable(display);
  Plot2DDisplayTestAccessor::disable(display);

  ASSERT_GE(backend->visibility.size(), 2U);
  EXPECT_TRUE(backend->visibility.front());
  EXPECT_FALSE(backend->visibility.back());
}

TEST(Plot2DDisplay, RenderOverlayUpdatesGeometryBeforeImageUpload)
{
  ensureQtApplication();
  Plot2DDisplay display;
  RecordingOverlayBackend * backend = nullptr;
  Plot2DDisplayTestAccessor::setOverlayBackendFactory(
    display,
    [&backend](std::string) {
      auto created = std::make_unique<RecordingOverlayBackend>();
      backend = created.get();
      return created;
    });
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  Plot2DDisplayTestAccessor::initializeOverlayBackend(display);
  ASSERT_NE(nullptr, backend);
  backend->events.clear();
  backend->geometries.clear();
  backend->image_sizes.clear();

  Plot2DDisplayTestAccessor::renderOverlay(display);

  const auto geometry_event =
    std::find(backend->events.begin(), backend->events.end(), "geometry");
  const auto image_event =
    std::find(backend->events.begin(), backend->events.end(), "image");
  ASSERT_NE(geometry_event, backend->events.end());
  ASSERT_NE(image_event, backend->events.end());
  EXPECT_LT(geometry_event, image_event);
  ASSERT_EQ(backend->geometries.size(), 1U);
  EXPECT_EQ(backend->geometries[0].width, 420);
  EXPECT_EQ(backend->geometries[0].height, 180);
  ASSERT_EQ(backend->image_sizes.size(), 1U);
  EXPECT_EQ(backend->image_sizes[0], std::make_pair(420, 180));
}

TEST(Plot2DDisplay, KeepsConfiguredSeriesRenderableBeforeTopicResolves)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/not_yet");
  findChild(series, "Field")->setValue("data");
  findChild(series, "Label")->setValue("Waiting");
  findChild(series, "Unit")->setValue("m/s");
  Plot2DDisplayTestAccessor::setTopics(display, TopicTypeMap{});

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  const auto renderable = Plot2DDisplayTestAccessor::renderableSeries(display);
  ASSERT_EQ(renderable.size(), 1U);
  EXPECT_TRUE(renderable[0].enabled);
  EXPECT_EQ(renderable[0].label, "Waiting");
  EXPECT_EQ(renderable[0].unit, "m/s");
  EXPECT_TRUE(renderable[0].samples.empty());
}

TEST(Plot2DDisplay, UsesSourcePathAsDefaultRenderableSeriesLabel)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  findChild(series, "Topic")->setValue("/cmd_vel");
  findChild(series, "Field")->setValue("linear/x");
  findChild(series, "Label")->setValue("");

  const auto renderable = Plot2DDisplayTestAccessor::renderableSeries(display);

  ASSERT_EQ(renderable.size(), 1U);
  EXPECT_EQ(renderable[0].label, "/cmd_vel/linear/x");
}

TEST(Plot2DDisplay, RenderSnapshotCombinesPropertyConfigAndControllerSamples)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  findChild(series, "Label")->setValue("Speed");
  findChild(series, "Unit")->setValue("m/s");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&callback](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> created_callback)
    {
      callback = std::move(created_callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);
  ASSERT_TRUE(callback);

  std_msgs::msg::Float64 message;
  message.data = 2.75;
  callback(serializeMessage(message));

  const auto snapshot = Plot2DDisplayTestAccessor::renderSnapshot(display);

  ASSERT_EQ(snapshot.config.series.size(), 1U);
  EXPECT_EQ(snapshot.config.series[0].topic, "/value");
  EXPECT_EQ(snapshot.config.series[0].field, "data");
  EXPECT_EQ(snapshot.config.series[0].label, "Speed");
  EXPECT_EQ(snapshot.config.series[0].unit, "m/s");
  ASSERT_EQ(snapshot.controller_state.series.size(), 1U);
  ASSERT_EQ(snapshot.controller_state.series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(snapshot.controller_state.series[0].samples.samples().front().value, 2.75);
}

TEST(Plot2DDisplay, HeaderStampRenderWindowUsesNewestSampleTime)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/pose");
  findChild(series, "Field")->setValue("pose/position/x");
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Time Source")->setValue(
    "Message Header Stamp");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/pose", {"geometry_msgs/msg/PoseStamped"}}});
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  geometry_msgs::msg::PoseStamped message;
  message.header.stamp.sec = 42;
  message.header.stamp.nanosec = 250000000;
  message.pose.position.x = 3.5;
  Plot2DDisplayTestAccessor::onSerializedMessage(
    display, "/pose", serializeMessage(message));

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);

  EXPECT_DOUBLE_EQ(settings.now, 42.25);
}

TEST(Plot2DDisplay, HeaderStampRenderWindowFallsBackBeforeSamplesArrive)
{
  ensureQtApplication();
  Plot2DDisplay display;
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Time Source")->setValue(
    "Message Header Stamp");

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);

  EXPECT_GT(settings.now, 1000.0);
}

TEST(Plot2DDisplay, SerializedMessageCallbackDoesNotQueueImmediateRvizRender)
{
  const std::string body = functionBody(
    plotDisplaySource(),
    "void Plot2DDisplay::onSerializedMessage_(");

  ASSERT_FALSE(body.empty());
  EXPECT_EQ(body.find("renderOverlay_();"), std::string::npos);
  EXPECT_EQ(body.find("queueRender();"), std::string::npos);
}

TEST(Plot2DDisplay, UpdateRefreshDoesNotQueueAnotherRvizFrame)
{
  const std::string body = functionBody(
    plotDisplaySource(),
    "void Plot2DDisplay::update(");

  ASSERT_FALSE(body.empty());
  EXPECT_NE(body.find("renderOverlay_(false);"), std::string::npos);
}
