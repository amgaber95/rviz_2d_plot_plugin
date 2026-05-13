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

TEST(Plot2DDisplay, ResolvedTopicCreatesGenericSubscription)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  int created = 0;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&created](
      const std::string & topic,
      const std::string & type,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)>)
    {
      ++created;
      EXPECT_EQ(topic, "/value");
      EXPECT_EQ(type, "std_msgs/msg/Float64");
      return rclcpp::GenericSubscription::SharedPtr{};
    });

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  const auto & state = Plot2DDisplayTestAccessor::controllerState(display);
  EXPECT_EQ(created, 1);
  EXPECT_EQ(state.status, PlotControllerStatus::Ok);
  ASSERT_EQ(state.series.size(), 1U);
  EXPECT_EQ(state.series[0].topic, "/value");
  EXPECT_EQ(state.series[0].type, "std_msgs/msg/Float64");
}

TEST(Plot2DDisplay, UsesConfiguredQosForGenericSubscriptions)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  auto * qos = findChild(&display, "QoS");
  ASSERT_NE(nullptr, qos);
  findChild(qos, "Reliability")->setValue("Best Effort");
  findChild(qos, "Durability")->setValue("Transient Local");
  findChild(qos, "Depth")->setValue(7);
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  std::optional<rmw_qos_profile_t> received_qos;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&received_qos](
      const std::string &,
      const std::string &,
      rclcpp::QoS qos,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)>)
    {
      received_qos = qos.get_rmw_qos_profile();
      return rclcpp::GenericSubscription::SharedPtr{};
    });

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  ASSERT_TRUE(received_qos.has_value());
  EXPECT_EQ(received_qos->reliability, RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
  EXPECT_EQ(received_qos->durability, RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
  EXPECT_EQ(received_qos->depth, 7U);
}

TEST(Plot2DDisplay, SubscriptionFactoryRunsAfterControllerLockIsReleased)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  bool factory_saw_unlocked_controller = false;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&display, &factory_saw_unlocked_controller](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)>)
    {
      factory_saw_unlocked_controller = Plot2DDisplayTestAccessor::canLockController(display);
      return rclcpp::GenericSubscription::SharedPtr{};
    });

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  EXPECT_TRUE(factory_saw_unlocked_controller);
}

TEST(Plot2DDisplay, SerializedMessageAppendsControllerSample)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  std_msgs::msg::Float64 message;
  message.data = 12.5;
  Plot2DDisplayTestAccessor::onSerializedMessage(display, serializeMessage(message));

  const auto & state = Plot2DDisplayTestAccessor::controllerState(display);
  ASSERT_EQ(state.series.size(), 1U);
  ASSERT_TRUE(state.series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(state.series[0].latest_value.value(), 12.5);
  EXPECT_EQ(state.series[0].samples.size(), 1U);
}

TEST(Plot2DDisplay, SeriesEnableTogglePreservesControllerSamples)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  auto * enabled = dynamic_cast<rviz_common::properties::BoolProperty *>(series);
  ASSERT_NE(nullptr, enabled);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  std_msgs::msg::Float64 message;
  message.data = 12.5;
  Plot2DDisplayTestAccessor::onSerializedMessage(display, serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);

  enabled->setBool(false);
  processQtEvents();

  const auto disabled_snapshot = Plot2DDisplayTestAccessor::renderSnapshot(display);
  ASSERT_EQ(disabled_snapshot.config.series.size(), 1U);
  ASSERT_EQ(disabled_snapshot.controller_state.series.size(), 1U);
  EXPECT_FALSE(disabled_snapshot.config.series[0].enabled);
  EXPECT_EQ(
    disabled_snapshot.controller_state.series[0].status,
    PlotControllerStatus::Disabled);
  ASSERT_EQ(disabled_snapshot.controller_state.series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(
    disabled_snapshot.controller_state.series[0].samples.samples().front().value, 12.5);

  enabled->setBool(true);
  processQtEvents();

  const auto enabled_snapshot = Plot2DDisplayTestAccessor::renderSnapshot(display);
  ASSERT_EQ(enabled_snapshot.config.series.size(), 1U);
  ASSERT_EQ(enabled_snapshot.controller_state.series.size(), 1U);
  EXPECT_TRUE(enabled_snapshot.config.series[0].enabled);
  EXPECT_EQ(enabled_snapshot.controller_state.series[0].status, PlotControllerStatus::Ok);
  ASSERT_EQ(enabled_snapshot.controller_state.series[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(
    enabled_snapshot.controller_state.series[0].samples.samples().front().value, 12.5);
  ASSERT_TRUE(enabled_snapshot.controller_state.series[0].latest_value.has_value());
  EXPECT_DOUBLE_EQ(enabled_snapshot.controller_state.series[0].latest_value.value(), 12.5);
}

TEST(Plot2DDisplay, SeriesDragDropPreservesControllerSamplesBySource)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_root = Plot2DDisplayTestAccessor::seriesRoot(display);
  auto * series_count = findChild(series_root, "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(2);
  auto * series_1 = findChild(series_root, "Series 1");
  auto * series_2 = findChild(series_root, "Series 2");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  findChild(series_1, "Topic")->setValue("/cmd_vel");
  findChild(series_1, "Field")->setValue("linear/x");
  findChild(series_1, "Label")->setValue("Linear X");
  findChild(series_2, "Topic")->setValue("/cmd_vel");
  findChild(series_2, "Field")->setValue("angular/z");
  findChild(series_2, "Label")->setValue("Angular Z");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/cmd_vel", {"geometry_msgs/msg/Twist"}}});
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  geometry_msgs::msg::Twist message;
  message.linear.x = 1.5;
  message.angular.z = -0.4;
  Plot2DDisplayTestAccessor::onSerializedMessage(
    display, "/cmd_vel", serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[1].samples.size(), 1U);

  rviz_common::properties::Property * moved = series_root->takeChildAt(2);
  ASSERT_NE(nullptr, moved);
  series_root->addChild(moved, 1);
  processQtEvents();

  const auto snapshot = Plot2DDisplayTestAccessor::renderSnapshot(display);
  ASSERT_EQ(snapshot.config.series.size(), 2U);
  ASSERT_EQ(snapshot.controller_state.series.size(), 2U);
  EXPECT_EQ(snapshot.config.series[0].field, "angular/z");
  EXPECT_EQ(snapshot.config.series[1].field, "linear/x");
  ASSERT_EQ(snapshot.controller_state.series[0].samples.size(), 1U);
  ASSERT_EQ(snapshot.controller_state.series[1].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(snapshot.controller_state.series[0].samples.samples().front().value, -0.4);
  EXPECT_DOUBLE_EQ(snapshot.controller_state.series[1].samples.samples().front().value, 1.5);
}

TEST(Plot2DDisplay, ReferenceAppearanceEditsDoNotRecreateSubscriptions)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  ASSERT_NE(nullptr, series);
  ASSERT_NE(nullptr, references_root);
  findChild(references_root, "Reference Count")->setValue(1);
  auto * reference = findChild(references_root, "Reference 1");
  ASSERT_NE(nullptr, reference);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  int created = 0;
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&created, &callback](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> created_callback)
    {
      ++created;
      callback = std::move(created_callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);
  ASSERT_EQ(created, 1);
  ASSERT_TRUE(callback);

  std_msgs::msg::Float64 message;
  message.data = 4.0;
  callback(serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  created = 0;

  findChild(reference, "Y Value")->setValue(2.0);
  findChild(reference, "Tolerance")->setValue(0.25);
  findChild(reference, "Label")->setValue("Limit");
  findChild(reference, "Color")->setValue(QColor(20, 200, 80));
  findChild(reference, "Alpha")->setValue(0.5);
  findChild(reference, "Line Width")->setValue(2.0);
  findChild(reference, "Line Style")->setValue("Dash");

  EXPECT_EQ(created, 0);
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  const auto references = Plot2DDisplayTestAccessor::renderableReferences(display);
  ASSERT_EQ(references.size(), 1U);
  EXPECT_DOUBLE_EQ(references[0].value, 2.0);
  EXPECT_NEAR(references[0].tolerance, 0.25, 1e-6);
  EXPECT_EQ(references[0].label, "Limit");
  EXPECT_EQ(references[0].color.red(), 20);
  EXPECT_EQ(references[0].color.green(), 200);
  EXPECT_EQ(references[0].color.blue(), 80);
  EXPECT_NEAR(references[0].color.alphaF(), 0.5, 0.01);
  EXPECT_DOUBLE_EQ(references[0].line_width, 2.0);
  EXPECT_EQ(references[0].line_style, rviz_2d_plot_plugin::LineStyle::Dash);
}

TEST(Plot2DDisplay, SeriesAppearanceEditsDoNotRecreateSubscriptions)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  findChild(series, "Label")->setValue("Speed");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  int created = 0;
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&created, &callback](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> created_callback)
    {
      ++created;
      callback = std::move(created_callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);
  ASSERT_EQ(created, 1);
  ASSERT_TRUE(callback);

  std_msgs::msg::Float64 message;
  message.data = 3.5;
  callback(serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  created = 0;

  findChild(series, "Label")->setValue("Velocity");
  findChild(series, "Unit")->setValue("m/s");
  findChild(series, "Color")->setValue(QColor(255, 80, 20));
  findChild(series, "Line Width")->setValue(3.0);
  findChild(series, "Line Alpha")->setValue(0.5);
  findChild(series, "Line Style")->setValue("Dot");
  findChild(series, "Plot Style")->setValue("Step");

  EXPECT_EQ(created, 0);
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  const auto renderable = Plot2DDisplayTestAccessor::renderableSeries(display);
  ASSERT_EQ(renderable.size(), 1U);
  EXPECT_EQ(renderable[0].label, "Velocity");
  EXPECT_EQ(renderable[0].unit, "m/s");
  EXPECT_EQ(renderable[0].color.red(), 255);
  EXPECT_EQ(renderable[0].color.green(), 80);
  EXPECT_EQ(renderable[0].color.blue(), 20);
  EXPECT_NEAR(renderable[0].color.alphaF(), 0.5, 0.01);
  EXPECT_DOUBLE_EQ(renderable[0].line_width, 3.0);
  EXPECT_EQ(renderable[0].line_style, rviz_2d_plot_plugin::LineStyle::Dot);
  EXPECT_EQ(renderable[0].plot_style, rviz_2d_plot_plugin::PlotStyle::Step);
  ASSERT_EQ(renderable[0].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(renderable[0].samples.front().value, 3.5);
}

TEST(Plot2DDisplay, OverlayRenderEditsDoNotRecreateSubscriptions)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  int created = 0;
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&created, &callback](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> created_callback)
    {
      ++created;
      callback = std::move(created_callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);
  ASSERT_EQ(created, 1);
  ASSERT_TRUE(callback);

  std_msgs::msg::Float64 message;
  message.data = 9.0;
  callback(serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  created = 0;

  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Min")->setValue(-2.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Max")->setValue(2.0);
  findChild(Plot2DDisplayTestAccessor::gridRoot(display), "Minor Divisions")->setValue(3);

  EXPECT_EQ(created, 0);
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.width, 420);
  EXPECT_EQ(settings.height, 180);
  EXPECT_EQ(settings.y_scale_mode, AxisScaleMode::Fixed);
  EXPECT_DOUBLE_EQ(settings.fixed_y_min, -2.0);
  EXPECT_DOUBLE_EQ(settings.fixed_y_max, 2.0);
  EXPECT_EQ(settings.minor_grid_divisions, 3);
}

TEST(Plot2DDisplay, DoesNotRecreateHealthySubscriptionsDuringRetryUpdate)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});

  int created = 0;
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&created, &callback](
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> created_callback)
    {
      ++created;
      callback = std::move(created_callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);
  ASSERT_EQ(created, 1);
  ASSERT_TRUE(callback);

  std_msgs::msg::Float64 message;
  message.data = 12.5;
  callback(serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);

  Plot2DDisplayTestAccessor::update(display, 1.1F, 1.1F);

  EXPECT_EQ(created, 1);
  EXPECT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);
}

TEST(Plot2DDisplay, CreatesSubscriptionsForMultipleSeriesTopics)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_count =
    findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count");
  ASSERT_NE(nullptr, series_count);
  series_count->setValue(2);

  auto * series_1 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  auto * series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  findChild(series_1, "Topic")->setValue("/left");
  findChild(series_1, "Field")->setValue("data");
  findChild(series_2, "Topic")->setValue("/right");
  findChild(series_2, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display,
    TopicTypeMap{
    {"/left", {"std_msgs/msg/Float64"}},
    {"/right", {"std_msgs/msg/Float64"}},
  });

  std::map<std::string, std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)>>
  callbacks;
  Plot2DDisplayTestAccessor::setSubscriptionFactory(
    display,
    [&callbacks](
      const std::string & topic,
      const std::string & type,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback)
    {
      EXPECT_EQ(type, "std_msgs/msg/Float64");
      callbacks[topic] = std::move(callback);
      return rclcpp::GenericSubscription::SharedPtr{};
    });

  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  ASSERT_EQ(callbacks.size(), 2U);
  std_msgs::msg::Float64 left;
  left.data = 1.25;
  callbacks.at("/left")(serializeMessage(left));
  std_msgs::msg::Float64 right;
  right.data = -2.5;
  callbacks.at("/right")(serializeMessage(right));

  const auto & state = Plot2DDisplayTestAccessor::controllerState(display);
  ASSERT_EQ(state.series.size(), 2U);
  ASSERT_EQ(state.series[0].samples.size(), 1U);
  ASSERT_EQ(state.series[1].samples.size(), 1U);
  EXPECT_DOUBLE_EQ(state.series[0].samples.samples().front().value, 1.25);
  EXPECT_DOUBLE_EQ(state.series[1].samples.samples().front().value, -2.5);
}

TEST(Plot2DDisplay, ClearHistoryPropertyClearsSamplesAndResetsCheckbox)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  findChild(series, "Topic")->setValue("/value");
  findChild(series, "Field")->setValue("data");
  Plot2DDisplayTestAccessor::setTopics(
    display, TopicTypeMap{{"/value", {"std_msgs/msg/Float64"}}});
  Plot2DDisplayTestAccessor::resolveAndSubscribe(display);

  std_msgs::msg::Float64 message;
  message.data = 7.0;
  Plot2DDisplayTestAccessor::onSerializedMessage(display, serializeMessage(message));
  ASSERT_EQ(
    Plot2DDisplayTestAccessor::controllerState(display).series[0].samples.size(), 1U);

  Plot2DDisplayTestAccessor::clearHistory(display)->setValue(true);

  const auto & state = Plot2DDisplayTestAccessor::controllerState(display);
  EXPECT_TRUE(state.series[0].samples.empty());
  EXPECT_FALSE(state.series[0].latest_value.has_value());
  EXPECT_FALSE(Plot2DDisplayTestAccessor::clearHistory(display)->getBool());
}
