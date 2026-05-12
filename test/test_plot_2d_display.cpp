// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QImage>
#include <QString>
#include <QStyleOptionViewItem>
#include <Qt>

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <std_msgs/msg/float64.hpp>

#include <rviz_common/config.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/property.hpp>
#include <rviz_common/properties/string_property.hpp>

#include "overlay_backend.hpp"

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

  static void setOverlayBackendFactory(
    Plot2DDisplay & display,
    std::function<std::unique_ptr<OverlayBackend>(std::string)> factory)
  {
    display.overlay_backend_factory_ = std::move(factory);
  }

  static void initializeOverlayBackend(Plot2DDisplay & display)
  {
    display.initializeOverlayBackend_();
  }

  static void enable(Plot2DDisplay & display)
  {
    display.onEnable();
  }

  static void disable(Plot2DDisplay & display)
  {
    display.onDisable();
  }

  static void renderOverlay(Plot2DDisplay & display)
  {
    display.renderOverlay_();
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

  static void onSerializedMessage(
    Plot2DDisplay & display,
    const std::string & topic,
    std::shared_ptr<rclcpp::SerializedMessage> message)
  {
    display.onSerializedMessage_(topic, std::move(message));
  }

  static const Plot2DControllerState & controllerState(Plot2DDisplay & display)
  {
    return display.controller_.state();
  }

  static bool canLockController(Plot2DDisplay & display)
  {
    std::unique_lock<std::mutex> lock(display.controller_mutex_, std::try_to_lock);
    return lock.owns_lock();
  }

  static std::vector<RenderableSeries> renderableSeries(Plot2DDisplay & display)
  {
    return display.renderableSeries_();
  }

  static auto renderSnapshot(Plot2DDisplay & display)
  {
    return display.renderSnapshot_();
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
using rviz_2d_plot_plugin::HorizontalAlignment;
using rviz_2d_plot_plugin::LegendPosition;
using rviz_2d_plot_plugin::OverlayBackend;
using rviz_2d_plot_plugin::OverlayBackendResult;
using rviz_2d_plot_plugin::OverlayGeometry;
using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::PlotMode;
using rviz_2d_plot_plugin::Plot2DDisplay;
using rviz_2d_plot_plugin::Plot2DDisplayTestAccessor;
using rviz_2d_plot_plugin::TimeSource;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::VerticalAlignment;
using rviz_2d_plot_plugin::XAxisMode;
using rviz_2d_plot_plugin::XYAxisScaleMode;
using rviz_2d_plot_plugin::XYHistoryMode;

class RecordingOverlayBackend final : public OverlayBackend
{
public:
  OverlayBackendResult initialize(Ogre::SceneManager * scene_manager) override
  {
    ++initialize_calls;
    initialized_scene_manager = scene_manager;
    return {};
  }

  OverlayBackendResult setGeometry(const OverlayGeometry & geometry) override
  {
    events.push_back("geometry");
    geometries.push_back(geometry);
    return {};
  }

  void setVisible(const bool visible) override
  {
    events.push_back(visible ? "show" : "hide");
    visibility.push_back(visible);
  }

  bool isReady() const override
  {
    return true;
  }

  OverlayBackendResult updateImage(const QImage & image) override
  {
    events.push_back("image");
    image_sizes.push_back({image.width(), image.height()});
    return {};
  }

  int initialize_calls{0};
  Ogre::SceneManager * initialized_scene_manager{nullptr};
  std::vector<OverlayGeometry> geometries;
  std::vector<bool> visibility;
  std::vector<std::pair<int, int>> image_sizes;
  std::vector<std::string> events;
};

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

void processQtEvents()
{
  QApplication::processEvents();
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

std::vector<QString> completionsFor(QCompleter * completer, const QString & prefix)
{
  std::vector<QString> completions;
  if (!completer) {
    return completions;
  }

  completer->setCompletionPrefix(prefix);
  for (int row = 0; completer->setCurrentRow(row); ++row) {
    completions.push_back(completer->currentCompletion());
  }
  return completions;
}

std::string plotDisplaySource()
{
  const std::filesystem::path test_file{__FILE__};
  const std::filesystem::path source_file =
    test_file.parent_path().parent_path() / "src" / "plot_2d_display.cpp";
  std::ifstream input(source_file);
  return std::string(
    std::istreambuf_iterator<char>(input),
    std::istreambuf_iterator<char>());
}

std::string functionBody(
  const std::string & source,
  const std::string & signature)
{
  const std::size_t signature_start = source.find(signature);
  if (signature_start == std::string::npos) {
    return {};
  }

  const std::size_t body_start = source.find('{', signature_start);
  if (body_start == std::string::npos) {
    return {};
  }

  int depth = 0;
  for (std::size_t i = body_start; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}') {
      --depth;
      if (depth == 0) {
        return source.substr(body_start, i - body_start + 1);
      }
    }
  }
  return {};
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
  EXPECT_NE(nullptr, findChild(&display, "Plot Mode"));

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  EXPECT_NE(nullptr, findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count"));
  EXPECT_TRUE(series->getValue().canConvert<bool>());
  EXPECT_EQ(nullptr, findChild(series, "Enabled"));
  EXPECT_NE(nullptr, findChild(series, "Action"));
  EXPECT_NE(nullptr, findChild(series, "Topic"));
  EXPECT_NE(nullptr, findChild(series, "X Field"));
  EXPECT_NE(nullptr, findChild(series, "Y Field"));
  EXPECT_NE(nullptr, findChild(series, "Field"));
  EXPECT_FALSE(findChild(series, "Field")->getHidden());
  EXPECT_TRUE(findChild(series, "X Field")->getHidden());
  EXPECT_TRUE(findChild(series, "Y Field")->getHidden());
  EXPECT_NE(nullptr, findChild(series, "Label"));
  EXPECT_NE(nullptr, findChild(series, "Unit"));
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
  EXPECT_NE(nullptr, findChild(time, "XY History Mode"));
  EXPECT_TRUE(findChild(time, "XY History Mode")->getHidden());
  EXPECT_NE(nullptr, findChild(time, "Refresh Rate"));

  auto * x_axis = Plot2DDisplayTestAccessor::xAxisRoot(display);
  EXPECT_TRUE(x_axis->getHidden());
  EXPECT_EQ(nullptr, findChild(x_axis, "Mode"));
  EXPECT_NE(nullptr, findChild(x_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Min"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Max"));
  EXPECT_NE(nullptr, findChild(x_axis, "Axis Scale"));

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
  EXPECT_NE(nullptr, findChild(references, "Preset"));
  EXPECT_NE(nullptr, findChild(references, "Preset Value"));
  EXPECT_NE(nullptr, findChild(references, "Preset Tolerance"));
  EXPECT_NE(nullptr, findChild(references, "Apply Preset"));
  EXPECT_NE(nullptr, findChild(references, "Reference Count"));

  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  EXPECT_NE(nullptr, findChild(legend, "Enabled"));
  EXPECT_NE(nullptr, findChild(legend, "Show Values"));
  EXPECT_NE(nullptr, findChild(legend, "Position"));
  EXPECT_NE(nullptr, findChild(legend, "X Offset"));
  EXPECT_NE(nullptr, findChild(legend, "Y Offset"));

  auto * layout = Plot2DDisplayTestAccessor::layoutRoot(display);
  EXPECT_NE(nullptr, findChild(layout, "Width"));
  EXPECT_NE(nullptr, findChild(layout, "Height"));
  EXPECT_NE(nullptr, findChild(layout, "X Offset"));
  EXPECT_NE(nullptr, findChild(layout, "Y Offset"));
  EXPECT_NE(nullptr, findChild(layout, "Horizontal Alignment"));
  EXPECT_NE(nullptr, findChild(layout, "Vertical Alignment"));

  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  EXPECT_NE(nullptr, findChild(style, "Background Color"));
  EXPECT_NE(nullptr, findChild(style, "Background Alpha"));
  EXPECT_NE(nullptr, findChild(style, "Axis Color"));
  EXPECT_NE(nullptr, findChild(style, "Grid Color"));
  EXPECT_NE(nullptr, findChild(style, "Text Color"));
  EXPECT_NE(nullptr, findChild(style, "Font Size"));
}

TEST(Plot2DDisplay, PlotModeSwitchesBetweenTimeAndXYSeriesFields)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  auto * field = findChild(series, "Field");
  auto * x_field = findChild(series, "X Field");
  auto * y_field = findChild(series, "Y Field");
  ASSERT_NE(nullptr, field);
  ASSERT_NE(nullptr, x_field);
  ASSERT_NE(nullptr, y_field);
  auto * xy_history_mode =
    findChild(Plot2DDisplayTestAccessor::timeRoot(display), "XY History Mode");
  ASSERT_NE(nullptr, xy_history_mode);

  EXPECT_FALSE(field->getHidden());
  EXPECT_TRUE(x_field->getHidden());
  EXPECT_TRUE(y_field->getHidden());
  EXPECT_TRUE(Plot2DDisplayTestAccessor::xAxisRoot(display)->getHidden());
  EXPECT_TRUE(xy_history_mode->getHidden());

  findChild(&display, "Plot Mode")->setValue("XY");

  EXPECT_TRUE(field->getHidden());
  EXPECT_FALSE(x_field->getHidden());
  EXPECT_FALSE(y_field->getHidden());
  EXPECT_FALSE(Plot2DDisplayTestAccessor::xAxisRoot(display)->getHidden());
  EXPECT_FALSE(xy_history_mode->getHidden());
}

TEST(Plot2DDisplay, SeriesRootShowsConfiguredSourceForPlotMode)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  EXPECT_EQ(series->getName(), "Series 1");
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "Series 1");
  EXPECT_TRUE(series->getValue().toBool());

  findChild(series, "Topic")->setValue("/cmd_vel");
  findChild(series, "Field")->setValue("linear/x");

  EXPECT_EQ(series->getName(), "Series 1");
  EXPECT_TRUE(series->getValue().toBool());
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "/cmd_vel/linear/x");

  findChild(&display, "Plot Mode")->setValue("XY");

  EXPECT_TRUE(series->getValue().toBool());
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "/cmd_vel");
}

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
    series,
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

  series->setValue(false);
  findChild(series, "Topic")->setValue("/odom");
  findChild(&display, "Plot Mode")->setValue("XY");
  findChild(series, "X Field")->setValue("pose/pose/position/x");
  findChild(series, "Y Field")->setValue("pose/pose/position/y");
  findChild(series, "Field")->setValue("pose/pose/position/y");
  findChild(series, "Label")->setValue("Odom Position");
  findChild(series, "Unit")->setValue("m");
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
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "XY History Mode")->setValue(
    "All Samples");
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Min")->setValue(-4.0);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Max")->setValue(4.0);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Axis Scale")->setValue("1:1");
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Min")->setValue(-2.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Max")->setValue(2.0);
  findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference Count")->setValue(1);
  auto * reference =
    findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference 1");
  ASSERT_NE(nullptr, reference);
  findChild(reference, "Enabled")->setValue(true);
  findChild(reference, "Value")->setValue(0.5);
  findChild(reference, "Tolerance")->setValue(0.2);
  findChild(reference, "Label")->setValue("Limit");
  findChild(reference, "Color")->setValue(QColor(255, 180, 60));
  findChild(reference, "Alpha")->setValue(0.6);
  findChild(reference, "Line Width")->setValue(1.5);
  findChild(reference, "Line Style")->setValue("Dot");
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "X Offset")->setValue(20);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Y Offset")->setValue(30);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Horizontal Alignment")->setValue(
    "Center");
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Vertical Alignment")->setValue(
    "Bottom");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 1U);
  EXPECT_FALSE(config.series[0].enabled);
  EXPECT_EQ(config.series[0].topic, "/odom");
  EXPECT_EQ(config.series[0].x_field, "pose/pose/position/x");
  EXPECT_EQ(config.series[0].y_field, "pose/pose/position/y");
  EXPECT_EQ(config.series[0].field, "pose/pose/position/y");
  EXPECT_EQ(config.series[0].label, "Odom Position");
  EXPECT_EQ(config.series[0].unit, "m");
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
  EXPECT_EQ(config.time.xy_history_mode, XYHistoryMode::AllSamples);
  EXPECT_EQ(config.plot_mode, PlotMode::XY);
  EXPECT_EQ(config.x_axis.mode, XAxisMode::Field);
  EXPECT_EQ(config.x_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.x_axis.axis_scale_mode, XYAxisScaleMode::Equal);
  EXPECT_EQ(config.x_axis.fixed_min, -4.0);
  EXPECT_EQ(config.x_axis.fixed_max, 4.0);
  EXPECT_EQ(config.y_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.y_axis.fixed_min, -2.0);
  EXPECT_EQ(config.y_axis.fixed_max, 2.0);
  ASSERT_EQ(config.references.size(), 1U);
  EXPECT_TRUE(config.references[0].enabled);
  EXPECT_DOUBLE_EQ(config.references[0].value, 0.5);
  EXPECT_NEAR(config.references[0].tolerance, 0.2, 1e-6);
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
  EXPECT_EQ(config.layout.horizontal_alignment, HorizontalAlignment::Center);
  EXPECT_EQ(config.layout.vertical_alignment, VerticalAlignment::Bottom);
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
  auto * preset = findChild(references_root, "Preset");
  auto * preset_value = findChild(references_root, "Preset Value");
  auto * preset_tolerance = findChild(references_root, "Preset Tolerance");
  auto * apply_preset = findChild(references_root, "Apply Preset");
  ASSERT_NE(nullptr, preset);
  ASSERT_NE(nullptr, preset_value);
  ASSERT_NE(nullptr, preset_tolerance);
  ASSERT_NE(nullptr, apply_preset);
  preset->setValue("Tolerance Band");
  preset_value->setValue(1.0);
  preset_tolerance->setValue(0.25);

  EXPECT_EQ(reference_count->getValue().toInt(), 1);
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 2"));

  apply_preset->setValue(true);

  EXPECT_FALSE(apply_preset->getValue().toBool());
  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  EXPECT_DOUBLE_EQ(findChild(reference_1, "Value")->getValue().toDouble(), 0.25);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "Existing");
  EXPECT_DOUBLE_EQ(findChild(reference_2, "Value")->getValue().toDouble(), 1.0);
  EXPECT_NEAR(findChild(reference_2, "Tolerance")->getValue().toDouble(), 0.25, 1e-6);
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "Target");
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 3"));
  EXPECT_EQ(preset->getValue().toString(), "Tolerance Band");

  preset->setValue("Unknown Preset");
  apply_preset->setValue(true);

  EXPECT_FALSE(apply_preset->getValue().toBool());
  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 3"));
}

TEST(Plot2DDisplay, ReferenceActionDeletesReference)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(2);
  auto * reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  findChild(reference_1, "Value")->setValue(0.25);
  findChild(reference_1, "Label")->setValue("First");
  findChild(reference_2, "Value")->setValue(0.75);
  findChild(reference_2, "Tolerance")->setValue(0.1);
  findChild(reference_2, "Label")->setValue("Second");

  auto * action = findChild(reference_1, "Action");
  ASSERT_NE(nullptr, action);
  action->setValue("Delete");
  processQtEvents();

  EXPECT_EQ(reference_count->getValue().toInt(), 1);
  reference_1 = findChild(references_root, "Reference 1");
  ASSERT_NE(nullptr, reference_1);
  EXPECT_DOUBLE_EQ(findChild(reference_1, "Value")->getValue().toDouble(), 0.75);
  EXPECT_NEAR(findChild(reference_1, "Tolerance")->getValue().toDouble(), 0.1, 1e-6);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "Second");
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 2"));
}

TEST(Plot2DDisplay, MapsLegendPropertiesToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  findChild(legend, "Enabled")->setValue(false);
  findChild(legend, "Show Values")->setValue(false);
  findChild(legend, "Position")->setValue("Bottom Right");
  findChild(legend, "X Offset")->setValue(12);
  findChild(legend, "Y Offset")->setValue(8);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_FALSE(settings.show_legend);
  EXPECT_FALSE(settings.show_latest_values);
  EXPECT_EQ(settings.legend_position, LegendPosition::BottomRight);
  EXPECT_EQ(settings.legend_x_offset, 12);
  EXPECT_EQ(settings.legend_y_offset, 8);
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

TEST(Plot2DDisplay, MapsBackgroundAlphaToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  findChild(style, "Background Color")->setValue(QColor(10, 20, 30));
  findChild(style, "Background Alpha")->setValue(0.25);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.background_color.red(), 10);
  EXPECT_EQ(settings.background_color.green(), 20);
  EXPECT_EQ(settings.background_color.blue(), 30);
  EXPECT_NEAR(settings.background_color.alphaF(), 0.25, 1e-3);
}

TEST(Plot2DDisplay, MapsFontSizeToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  findChild(style, "Font Size")->setValue(12);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.font_size, 12);
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
  processQtEvents();

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
  processQtEvents();
  series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_EQ(findChild(series_2, "Label")->getValue().toString(), "Second");
  EXPECT_EQ(findChild(series_3, "Label")->getValue().toString(), "First Copy");

  action = findChild(series_3, "Action");
  ASSERT_NE(nullptr, action);
  action->setValue("Delete");
  processQtEvents();

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

TEST(Plot2DDisplay, LoadsDynamicSeriesAndReferenceCountsBeforeChildren)
{
  ensureQtApplication();
  Plot2DDisplay display;

  rviz_common::Config config;
  auto series = config.mapMakeChild("Series");
  series.mapSetValue("Series Count", 2);
  auto series_1 = series.mapMakeChild("Series 1");
  series_1.mapSetValue("Topic", "/cmd_vel");
  series_1.mapSetValue("Field", "linear/x");
  series_1.mapSetValue("Label", "Linear X");
  auto series_2 = series.mapMakeChild("Series 2");
  series_2.mapSetValue("Topic", "/cmd_vel");
  series_2.mapSetValue("Field", "angular/z");
  series_2.mapSetValue("Label", "Angular Z");

  auto references = config.mapMakeChild("References");
  references.mapSetValue("Reference Count", 1);
  auto reference_1 = references.mapMakeChild("Reference 1");
  reference_1.mapSetValue("Value", 3.0);
  reference_1.mapSetValue("Label", "Upper Limit");

  testing::internal::CaptureStdout();
  display.load(config);
  const std::string load_output = testing::internal::GetCapturedStdout();

  const Plot2DConfig loaded = Plot2DDisplayTestAccessor::configFromProperties(display);
  ASSERT_EQ(loaded.series.size(), 2U);
  EXPECT_EQ(loaded.series[0].topic, "/cmd_vel");
  EXPECT_EQ(loaded.series[0].field, "linear/x");
  EXPECT_EQ(loaded.series[0].label, "Linear X");
  EXPECT_EQ(loaded.series[1].topic, "/cmd_vel");
  EXPECT_EQ(loaded.series[1].field, "angular/z");
  EXPECT_EQ(loaded.series[1].label, "Angular Z");
  ASSERT_EQ(loaded.references.size(), 1U);
  EXPECT_DOUBLE_EQ(loaded.references[0].value, 3.0);
  EXPECT_EQ(loaded.references[0].label, "Upper Limit");
  EXPECT_EQ(load_output.find("unexpected QVariant type"), std::string::npos);
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

TEST(Plot2DDisplay, SerializedMessageCallbackDoesNotRenderOverlayTexture)
{
  const std::string body = functionBody(
    plotDisplaySource(),
    "void Plot2DDisplay::onSerializedMessage_(");

  ASSERT_FALSE(body.empty());
  EXPECT_EQ(body.find("renderOverlay_();"), std::string::npos);
  EXPECT_NE(body.find("queueRender();"), std::string::npos);
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

TEST(Plot2DDisplay, TopicAndFieldEditorsFilterOptionsByContainsWhileTyping)
{
  ensureQtApplication();
  Plot2DDisplay display;
  Plot2DDisplayTestAccessor::setTopics(
    display,
    TopicTypeMap{{"/cmd_vel_out", {"geometry_msgs/msg/Twist"}}});

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  auto * topic_property =
    qobject_cast<rviz_common::properties::EditableEnumProperty *>(
    findChild(series, "Topic"));
  ASSERT_NE(nullptr, topic_property);

  QStyleOptionViewItem option;
  std::unique_ptr<QWidget> topic_editor(topic_property->createEditor(nullptr, option));
  auto * topic_combo = qobject_cast<QComboBox *>(topic_editor.get());
  ASSERT_NE(nullptr, topic_combo);
  ASSERT_NE(nullptr, topic_combo->completer());
  EXPECT_EQ(topic_combo->completer()->filterMode(), Qt::MatchContains);
  const std::vector<QString> topic_completions =
    completionsFor(topic_combo->completer(), "cmd");
  EXPECT_NE(
    std::find(topic_completions.begin(), topic_completions.end(), "/cmd_vel_out"),
    topic_completions.end());

  topic_property->setValue("/cmd_vel_out");
  auto * field_property =
    qobject_cast<rviz_common::properties::EditableEnumProperty *>(
    findChild(series, "Field"));
  ASSERT_NE(nullptr, field_property);

  std::unique_ptr<QWidget> field_editor(field_property->createEditor(nullptr, option));
  auto * field_combo = qobject_cast<QComboBox *>(field_editor.get());
  ASSERT_NE(nullptr, field_combo);
  ASSERT_NE(nullptr, field_combo->completer());
  EXPECT_EQ(field_combo->completer()->filterMode(), Qt::MatchContains);
  const std::vector<QString> field_completions =
    completionsFor(field_combo->completer(), "x");
  EXPECT_NE(
    std::find(field_completions.begin(), field_completions.end(), "linear/x"),
    field_completions.end());
}
