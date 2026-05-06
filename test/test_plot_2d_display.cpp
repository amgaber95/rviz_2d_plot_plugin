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
#include <map>
#include <memory>
#include <stdexcept>
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

namespace Ogre
{
class SceneManager;
}  // namespace Ogre

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

  static rviz_common::properties::Property * xAxisRoot(Plot2DDisplay & display)
  {
    return display.x_axis_root_property_;
  }

  static rviz_common::properties::Property * gridRoot(Plot2DDisplay & display)
  {
    return display.grid_root_property_;
  }

  static rviz_common::properties::Property * referencesRoot(Plot2DDisplay & display)
  {
    return display.references_root_property_;
  }

  static rviz_common::properties::Property * legendRoot(Plot2DDisplay & display)
  {
    return display.legend_root_property_;
  }

  static rviz_common::properties::Property * layoutRoot(Plot2DDisplay & display)
  {
    return display.layout_root_property_;
  }

  static Plot2DConfig configFromProperties(Plot2DDisplay & display)
  {
    return display.configFromProperties_();
  }

  static rviz_2d_plot_plugin::PlotRenderSettings renderSettingsFromProperties(
    Plot2DDisplay & display)
  {
    return display.renderSettingsFromProperties_();
  }

  static void setTopics(Plot2DDisplay & display, TopicTypeMap topics)
  {
    display.ros_graph_ops_.get_topic_names_and_types =
      [topics]() {
        return topics;
      };
  }

  static void setTopicProvider(Plot2DDisplay & display, std::function<TopicTypeMap()> provider)
  {
    display.ros_graph_ops_.get_topic_names_and_types = std::move(provider);
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

  static void setOverlayPreparer(
    Plot2DDisplay & display,
    std::function<void(Ogre::SceneManager *)> prepare)
  {
    display.overlay_backend_ops_.prepare_overlays = std::move(prepare);
  }

  static void prepareOverlayRendering(Plot2DDisplay & display)
  {
    display.prepareOverlayRendering_();
  }

  static void resolveAndSubscribe(Plot2DDisplay & display)
  {
    display.resolveAndSubscribe_();
  }

  static void update(Plot2DDisplay & display, const float wall_dt, const float ros_dt)
  {
    display.update(wall_dt, ros_dt);
  }

  static void onSerializedMessage(
    Plot2DDisplay & display,
    std::shared_ptr<rclcpp::SerializedMessage> message)
  {
    display.onSerializedMessage_("/value", std::move(message));
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
using rviz_2d_plot_plugin::LegendPosition;
using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::Plot2DDisplay;
using rviz_2d_plot_plugin::Plot2DDisplayTestAccessor;
using rviz_2d_plot_plugin::TimeSource;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::XAxisMode;

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
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::referencesRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::layoutRoot(display));
  EXPECT_EQ(findChild(&display, "Pause Plot"), Plot2DDisplayTestAccessor::pausePlot(display));
  EXPECT_EQ(findChild(&display, "Clear History"), Plot2DDisplayTestAccessor::clearHistory(display));

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  EXPECT_NE(nullptr, findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count"));
  EXPECT_NE(nullptr, findChild(series, "Enabled"));
  EXPECT_NE(nullptr, findChild(series, "Action"));
  EXPECT_NE(nullptr, findChild(series, "Topic"));
  EXPECT_NE(nullptr, findChild(series, "X Field"));
  EXPECT_NE(nullptr, findChild(series, "Field"));
  EXPECT_NE(nullptr, findChild(series, "Label"));
  EXPECT_NE(nullptr, findChild(series, "Color"));
  EXPECT_NE(nullptr, findChild(series, "Line Width"));
  EXPECT_NE(nullptr, findChild(series, "Line Alpha"));
  EXPECT_NE(nullptr, findChild(series, "Line Style"));
  EXPECT_NE(nullptr, findChild(series, "Plot Style"));
  EXPECT_NE(nullptr, findChild(series, "Value Scale"));
  EXPECT_NE(nullptr, findChild(series, "Value Offset"));

  auto * time = Plot2DDisplayTestAccessor::timeRoot(display);
  EXPECT_NE(nullptr, findChild(time, "Time Source"));
  EXPECT_NE(nullptr, findChild(time, "Window Seconds"));
  EXPECT_NE(nullptr, findChild(time, "Refresh Rate"));

  auto * x_axis = Plot2DDisplayTestAccessor::xAxisRoot(display);
  EXPECT_NE(nullptr, findChild(x_axis, "Mode"));
  EXPECT_NE(nullptr, findChild(x_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Min"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Max"));

  auto * y_axis = Plot2DDisplayTestAccessor::yAxisRoot(display);
  EXPECT_NE(nullptr, findChild(y_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Min"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Max"));

  auto * grid = Plot2DDisplayTestAccessor::gridRoot(display);
  ASSERT_NE(nullptr, grid);
  EXPECT_NE(nullptr, findChild(grid, "Major Grid"));
  EXPECT_NE(nullptr, findChild(grid, "Minor Grid"));
  EXPECT_NE(nullptr, findChild(grid, "X Major Ticks"));
  EXPECT_NE(nullptr, findChild(grid, "Y Major Ticks"));
  EXPECT_NE(nullptr, findChild(grid, "Minor Divisions"));

  auto * references = Plot2DDisplayTestAccessor::referencesRoot(display);
  EXPECT_NE(nullptr, findChild(references, "Add Preset"));
  EXPECT_NE(nullptr, findChild(references, "Preset Value"));
  EXPECT_NE(nullptr, findChild(references, "Reference Count"));

  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  EXPECT_NE(nullptr, findChild(legend, "Position"));

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

TEST(Plot2DDisplay, PreparesRvizOverlayRenderingBackend)
{
  ensureQtApplication();
  Plot2DDisplay display;
  int prepare_calls = 0;
  Plot2DDisplayTestAccessor::setOverlayPreparer(
    display,
    [&prepare_calls](Ogre::SceneManager * scene_manager) {
      (void)scene_manager;
      ++prepare_calls;
    });

  Plot2DDisplayTestAccessor::prepareOverlayRendering(display);

  EXPECT_EQ(prepare_calls, 1);
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
    findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Auto Scale"),
    findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale"),
    findChild(Plot2DDisplayTestAccessor::gridRoot(display), "Major Grid"),
    findChild(Plot2DDisplayTestAccessor::gridRoot(display), "Minor Grid"),
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
  findChild(series, "X Field")->setValue("linear/y");
  findChild(series, "Field")->setValue("linear/x");
  findChild(series, "Label")->setValue("Linear X");
  findChild(series, "Color")->setValue(QColor(255, 80, 20));
  findChild(series, "Line Width")->setValue(3.5);
  findChild(series, "Line Alpha")->setValue(0.45);
  findChild(series, "Line Style")->setValue("Dash");
  findChild(series, "Plot Style")->setValue("Step");
  findChild(series, "Value Scale")->setValue(2.5);
  findChild(series, "Value Offset")->setValue(-0.75);
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Window Seconds")->setValue(45.0);
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Refresh Rate")->setValue(12.0);
  auto * time_source = findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Time Source");
  ASSERT_NE(nullptr, time_source);
  time_source->setValue("Message Header Stamp");
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Mode")->setValue("Field");
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Min")->setValue(-4.0);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Max")->setValue(4.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Min")->setValue(-2.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Max")->setValue(2.0);
  findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference Count")->setValue(1);
  auto * reference =
    findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference 1");
  ASSERT_NE(nullptr, reference);
  findChild(reference, "Enabled")->setValue(true);
  findChild(reference, "Value")->setValue(0.5);
  findChild(reference, "Label")->setValue("Limit");
  findChild(reference, "Color")->setValue(QColor(255, 180, 60));
  findChild(reference, "Alpha")->setValue(0.6);
  findChild(reference, "Line Width")->setValue(1.5);
  findChild(reference, "Line Style")->setValue("Dot");
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "X Offset")->setValue(20);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Y Offset")->setValue(30);

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 1U);
  EXPECT_FALSE(config.series[0].enabled);
  EXPECT_EQ(config.series[0].topic, "/cmd_vel_out");
  EXPECT_EQ(config.series[0].x_field, "linear/y");
  EXPECT_EQ(config.series[0].field, "linear/x");
  EXPECT_EQ(config.series[0].label, "Linear X");
  EXPECT_EQ(config.series[0].color.red, 255);
  EXPECT_EQ(config.series[0].color.green, 80);
  EXPECT_EQ(config.series[0].color.blue, 20);
  EXPECT_DOUBLE_EQ(config.series[0].line_width, 3.5);
  EXPECT_NEAR(config.series[0].line_alpha, 0.45, 1e-6);
  EXPECT_EQ(config.series[0].line_style, rviz_2d_plot_plugin::LineStyle::Dash);
  EXPECT_EQ(config.series[0].plot_style, rviz_2d_plot_plugin::PlotStyle::Step);
  EXPECT_DOUBLE_EQ(config.series[0].value_scale, 2.5);
  EXPECT_DOUBLE_EQ(config.series[0].value_offset, -0.75);
  EXPECT_EQ(config.time.window_seconds, 45.0);
  EXPECT_EQ(config.time.refresh_rate_hz, 12.0);
  EXPECT_EQ(config.time.source, TimeSource::HeaderStamp);
  EXPECT_EQ(config.x_axis.mode, XAxisMode::Field);
  EXPECT_EQ(config.x_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.x_axis.fixed_min, -4.0);
  EXPECT_EQ(config.x_axis.fixed_max, 4.0);
  EXPECT_EQ(config.y_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.y_axis.fixed_min, -2.0);
  EXPECT_EQ(config.y_axis.fixed_max, 2.0);
  ASSERT_EQ(config.references.size(), 1U);
  EXPECT_TRUE(config.references[0].enabled);
  EXPECT_DOUBLE_EQ(config.references[0].value, 0.5);
  EXPECT_EQ(config.references[0].label, "Limit");
  EXPECT_EQ(config.references[0].color.red, 255);
  EXPECT_EQ(config.references[0].color.green, 180);
  EXPECT_EQ(config.references[0].color.blue, 60);
  EXPECT_NEAR(config.references[0].alpha, 0.6, 1e-6);
  EXPECT_DOUBLE_EQ(config.references[0].line_width, 1.5);
  EXPECT_EQ(config.references[0].line_style, rviz_2d_plot_plugin::LineStyle::Dot);
  EXPECT_EQ(config.layout.width, 420);
  EXPECT_EQ(config.layout.height, 180);
  EXPECT_EQ(config.layout.x_offset, 20);
  EXPECT_EQ(config.layout.y_offset, 30);
}

TEST(Plot2DDisplay, ReferencePresetAppendsNewReferences)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(1);
  auto * reference_1 = findChild(references_root, "Reference 1");
  ASSERT_NE(nullptr, reference_1);
  findChild(reference_1, "Value")->setValue(0.25);
  findChild(reference_1, "Label")->setValue("Existing");
  auto * preset_value = findChild(references_root, "Preset Value");
  auto * add_preset = findChild(references_root, "Add Preset");
  ASSERT_NE(nullptr, preset_value);
  ASSERT_NE(nullptr, add_preset);
  preset_value->setValue(0.75);

  add_preset->setValue("Upper Limit");

  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  EXPECT_DOUBLE_EQ(findChild(reference_1, "Value")->getValue().toDouble(), 0.25);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "Existing");
  EXPECT_DOUBLE_EQ(findChild(reference_2, "Value")->getValue().toDouble(), 0.75);
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "Upper Limit");
  EXPECT_EQ(add_preset->getValue().toString(), "None");
}

TEST(Plot2DDisplay, MapsLegendPositionPropertyToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  auto * position = findChild(legend, "Position");
  ASSERT_NE(nullptr, position);

  position->setValue("Bottom Right");

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.legend_position, LegendPosition::BottomRight);
}

TEST(Plot2DDisplay, MapsGridPropertiesToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * grid = Plot2DDisplayTestAccessor::gridRoot(display);
  ASSERT_NE(nullptr, grid);
  findChild(grid, "Major Grid")->setValue(false);
  findChild(grid, "Minor Grid")->setValue(true);
  findChild(grid, "X Major Ticks")->setValue(4);
  findChild(grid, "Y Major Ticks")->setValue(7);
  findChild(grid, "Minor Divisions")->setValue(2);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_FALSE(settings.show_major_grid);
  EXPECT_TRUE(settings.show_minor_grid);
  EXPECT_EQ(settings.x_major_tick_count, 4);
  EXPECT_EQ(settings.y_major_tick_count, 7);
  EXPECT_EQ(settings.minor_grid_divisions, 2);
}

TEST(Plot2DDisplay, AssignsDistinctDefaultColorsToNewSeries)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_count =
    findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(3);
  auto * series_1 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  auto * series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  auto * series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);

  auto * color_1 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_1, "Color"));
  auto * color_2 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_2, "Color"));
  auto * color_3 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_3, "Color"));
  ASSERT_NE(nullptr, color_1);
  ASSERT_NE(nullptr, color_2);
  ASSERT_NE(nullptr, color_3);

  EXPECT_NE(color_1->getColor(), color_2->getColor());
  EXPECT_NE(color_2->getColor(), color_3->getColor());
}

TEST(Plot2DDisplay, SeriesActionDuplicatesDeletesAndReordersSeries)
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
  findChild(series_1, "Label")->setValue("First");
  findChild(series_1, "Topic")->setValue("/first");
  findChild(series_1, "Field")->setValue("data");
  findChild(series_2, "Label")->setValue("Second");

  auto * action = findChild(series_1, "Action");
  ASSERT_NE(nullptr, action);
  action->setValue("Duplicate");

  EXPECT_EQ(series_count->getValue().toInt(), 3);
  series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  auto * series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_EQ(findChild(series_2, "Label")->getValue().toString(), "First Copy");
  EXPECT_EQ(findChild(series_2, "Topic")->getValue().toString(), "/first");
  EXPECT_EQ(findChild(series_3, "Label")->getValue().toString(), "Second");

  action = findChild(series_3, "Action");
  ASSERT_NE(nullptr, action);
  action->setValue("Move Up");
  series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_EQ(findChild(series_2, "Label")->getValue().toString(), "Second");
  EXPECT_EQ(findChild(series_3, "Label")->getValue().toString(), "First Copy");

  action = findChild(series_3, "Action");
  ASSERT_NE(nullptr, action);
  action->setValue("Delete");

  EXPECT_EQ(series_count->getValue().toInt(), 2);
  EXPECT_EQ(nullptr, findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3"));
}

TEST(Plot2DDisplay, BuildsPlotConfigFromMultipleSeriesProperties)
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

  findChild(series_1, "Topic")->setValue("/cmd_vel");
  findChild(series_1, "Field")->setValue("linear/x");
  findChild(series_1, "Label")->setValue("Linear X");
  findChild(series_2, "Topic")->setValue("/cmd_vel");
  findChild(series_2, "Field")->setValue("angular/z");
  findChild(series_2, "Label")->setValue("Angular Z");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 2U);
  EXPECT_EQ(config.series[0].topic, "/cmd_vel");
  EXPECT_EQ(config.series[0].field, "linear/x");
  EXPECT_EQ(config.series[0].label, "Linear X");
  EXPECT_EQ(config.series[1].topic, "/cmd_vel");
  EXPECT_EQ(config.series[1].field, "angular/z");
  EXPECT_EQ(config.series[1].label, "Angular Z");
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
  ASSERT_EQ(state.series.size(), 1U);
  EXPECT_EQ(state.series[0].topic, "/value");
  EXPECT_EQ(state.series[0].type, "std_msgs/msg/Float64");
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
