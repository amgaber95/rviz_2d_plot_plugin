// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <QApplication>
#include <QString>
#include <Qt>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <std_msgs/msg/float64.hpp>

#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/property.hpp>
#include <rviz_common/properties/string_property.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"
#include "rviz_2d_plot_plugin/plot_2d_display.hpp"
#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

namespace rviz_2d_plot_plugin
{

class Plot2DDisplayTestAccessor
{
public:
  static rviz_common::properties::BoolProperty * pausePlot(Plot2DDisplay & display)
  {
    return display.pause_plot_property_;
  }

  static rviz_common::properties::BoolProperty * clearHistory(Plot2DDisplay & display)
  {
    return display.clear_history_property_;
  }

  static rviz_common::properties::Property * seriesRoot(Plot2DDisplay & display)
  {
    return display.series_root_property_;
  }

  static rviz_common::properties::Property * timeRoot(Plot2DDisplay & display)
  {
    return display.time_root_property_;
  }

  static rviz_common::properties::Property * yAxisRoot(Plot2DDisplay & display)
  {
    return display.y_axis_root_property_;
  }

  static rviz_common::properties::Property * layoutRoot(Plot2DDisplay & display)
  {
    return display.layout_root_property_;
  }

  static Plot2DConfig configFromProperties(Plot2DDisplay & display)
  {
    return display.configFromProperties_();
  }

  static void setTopics(Plot2DDisplay & display, TopicTypeMap topics)
  {
    display.ros_graph_ops_.get_topic_names_and_types =
      [topics]() {
        return topics;
      };
  }

  static void setSubscriptionFactory(
    Plot2DDisplay & display,
    std::function<rclcpp::GenericSubscription::SharedPtr(
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)>)> factory)
  {
    display.subscription_factory_.create_generic_subscription = std::move(factory);
  }

  static void resolveAndSubscribe(Plot2DDisplay & display)
  {
    display.resolveAndSubscribe_();
  }

  static void onSerializedMessage(
    Plot2DDisplay & display,
    std::shared_ptr<rclcpp::SerializedMessage> message)
  {
    display.onSerializedMessage_(std::move(message));
  }

  static const Plot2DControllerState & controllerState(Plot2DDisplay & display)
  {
    return display.controller_.state();
  }

  static std::vector<std::string> topicOptions(Plot2DDisplay & display)
  {
    return display.topicOptions_();
  }

  static std::vector<std::string> fieldOptions(
    Plot2DDisplay & display,
    const std::string & topic)
  {
    return display.fieldOptionsForTopic_(topic);
  }
};

}  // namespace rviz_2d_plot_plugin

namespace
{

using rviz_2d_plot_plugin::AxisScaleMode;
using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::Plot2DDisplay;
using rviz_2d_plot_plugin::Plot2DDisplayTestAccessor;
using rviz_2d_plot_plugin::TopicTypeMap;

void ensureQtApplication()
{
  if (QApplication::instance()) {
    return;
  }

  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char app_name[] = "test_plot_2d_display";
  static char * argv[] = {app_name, nullptr};
  static QApplication application(argc, argv);
}

rviz_common::properties::Property * findChild(
  rviz_common::properties::Property * parent,
  const QString & name)
{
  if (!parent) {
    return nullptr;
  }
  for (int i = 0; i < parent->numChildren(); ++i) {
    auto * child = parent->childAt(i);
    if (child && child->getName() == name) {
      return child;
    }
  }
  return nullptr;
}

std::vector<QString> childNames(rviz_common::properties::Property * parent)
{
  std::vector<QString> names;
  if (!parent) {
    return names;
  }
  names.reserve(static_cast<std::size_t>(parent->numChildren()));
  for (int i = 0; i < parent->numChildren(); ++i) {
    auto * child = parent->childAt(i);
    if (child) {
      names.push_back(child->getName());
    }
  }
  return names;
}

template<typename MessageT>
std::shared_ptr<rclcpp::SerializedMessage> serializeMessage(const MessageT & message)
{
  rclcpp::Serialization<MessageT> serializer;
  auto serialized = std::make_shared<rclcpp::SerializedMessage>();
  serializer.serialize_message(&message, serialized.get());
  return serialized;
}

}  // namespace

TEST(Plot2DDisplay, CreatesMvpPropertyLayout)
{
  ensureQtApplication();
  Plot2DDisplay display;

  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::pausePlot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::clearHistory(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::seriesRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::timeRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::yAxisRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::layoutRoot(display));
  EXPECT_EQ(findChild(&display, "Pause Plot"), Plot2DDisplayTestAccessor::pausePlot(display));
  EXPECT_EQ(findChild(&display, "Clear History"), Plot2DDisplayTestAccessor::clearHistory(display));

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  EXPECT_NE(nullptr, findChild(series, "Enabled"));
  EXPECT_NE(nullptr, findChild(series, "Topic"));
  EXPECT_NE(nullptr, findChild(series, "Field"));
  EXPECT_NE(nullptr, findChild(series, "Label"));

  auto * time = Plot2DDisplayTestAccessor::timeRoot(display);
  EXPECT_NE(nullptr, findChild(time, "Window Seconds"));
  EXPECT_NE(nullptr, findChild(time, "Refresh Rate"));

  auto * y_axis = Plot2DDisplayTestAccessor::yAxisRoot(display);
  EXPECT_NE(nullptr, findChild(y_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Min"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Max"));

  auto * layout = Plot2DDisplayTestAccessor::layoutRoot(display);
  EXPECT_NE(nullptr, findChild(layout, "Width"));
  EXPECT_NE(nullptr, findChild(layout, "Height"));
  EXPECT_NE(nullptr, findChild(layout, "X Offset"));
  EXPECT_NE(nullptr, findChild(layout, "Y Offset"));

  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  EXPECT_NE(nullptr, findChild(style, "Background Color"));
  EXPECT_NE(nullptr, findChild(style, "Axis Color"));
  EXPECT_NE(nullptr, findChild(style, "Grid Color"));
  EXPECT_NE(nullptr, findChild(style, "Text Color"));
}

TEST(Plot2DDisplay, PlacesActionsBeforeConfigurationGroups)
{
  ensureQtApplication();
  Plot2DDisplay display;
  const auto names = childNames(&display);

  const auto pause = std::find(names.begin(), names.end(), "Pause Plot");
  const auto clear = std::find(names.begin(), names.end(), "Clear History");
  const auto series = std::find(names.begin(), names.end(), "Series");

  ASSERT_NE(names.end(), pause);
  ASSERT_NE(names.end(), clear);
  ASSERT_NE(names.end(), series);
  EXPECT_LT(std::distance(names.begin(), pause), std::distance(names.begin(), series));
  EXPECT_LT(std::distance(names.begin(), clear), std::distance(names.begin(), series));
}

TEST(Plot2DDisplay, BooleanPropertiesUseCheckboxEditing)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  std::vector<rviz_common::properties::Property *> bool_properties{
    Plot2DDisplayTestAccessor::pausePlot(display),
    Plot2DDisplayTestAccessor::clearHistory(display),
    findChild(series, "Enabled"),
    findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale"),
  };

  for (auto * property : bool_properties) {
    ASSERT_NE(nullptr, property);
    EXPECT_TRUE(property->getValue().canConvert<bool>());
    EXPECT_FALSE(property->getViewData(1, Qt::DisplayRole).isValid());
    EXPECT_TRUE(property->getViewData(1, Qt::CheckStateRole).isValid());
    EXPECT_TRUE(property->getViewFlags(1) & Qt::ItemIsUserCheckable);
  }
}

TEST(Plot2DDisplay, BuildsPlotConfigFromProperties)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  findChild(series, "Enabled")->setValue(false);
  findChild(series, "Topic")->setValue("/cmd_vel_out");
  findChild(series, "Field")->setValue("linear/x");
  findChild(series, "Label")->setValue("Linear X");
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Window Seconds")->setValue(45.0);
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Refresh Rate")->setValue(12.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Min")->setValue(-2.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Max")->setValue(2.0);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "X Offset")->setValue(20);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Y Offset")->setValue(30);

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 1U);
  EXPECT_FALSE(config.series[0].enabled);
  EXPECT_EQ(config.series[0].topic, "/cmd_vel_out");
  EXPECT_EQ(config.series[0].field, "linear/x");
  EXPECT_EQ(config.series[0].label, "Linear X");
  EXPECT_EQ(config.time.window_seconds, 45.0);
  EXPECT_EQ(config.time.refresh_rate_hz, 12.0);
  EXPECT_EQ(config.y_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.y_axis.fixed_min, -2.0);
  EXPECT_EQ(config.y_axis.fixed_max, 2.0);
  EXPECT_EQ(config.layout.width, 420);
  EXPECT_EQ(config.layout.height, 180);
  EXPECT_EQ(config.layout.x_offset, 20);
  EXPECT_EQ(config.layout.y_offset, 30);
}

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
  EXPECT_EQ(state.topic, "/value");
  EXPECT_EQ(state.type, "std_msgs/msg/Float64");
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
  ASSERT_TRUE(state.latest_value.has_value());
  EXPECT_DOUBLE_EQ(state.latest_value.value(), 12.5);
  EXPECT_EQ(state.samples.size(), 1U);
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
  ASSERT_EQ(Plot2DDisplayTestAccessor::controllerState(display).samples.size(), 1U);

  Plot2DDisplayTestAccessor::clearHistory(display)->setValue(true);

  const auto & state = Plot2DDisplayTestAccessor::controllerState(display);
  EXPECT_TRUE(state.samples.empty());
  EXPECT_FALSE(state.latest_value.has_value());
  EXPECT_FALSE(Plot2DDisplayTestAccessor::clearHistory(display)->getBool());
}

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
