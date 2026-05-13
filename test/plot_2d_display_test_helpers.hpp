// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef PLOT_2D_DISPLAY_TEST_HELPERS_HPP_
#define PLOT_2D_DISPLAY_TEST_HELPERS_HPP_

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
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
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
#include "plot_2d_subscription_manager.hpp"

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
    display.subscription_manager_->setFactory(std::move(factory));
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

  static std::vector<RenderableReference> renderableReferences(Plot2DDisplay & display)
  {
    return display.renderableReferences_();
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

namespace rviz_2d_plot_plugin::test
{

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

inline void ensureQtApplication()
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

inline void processQtEvents()
{
  QApplication::processEvents();
}

inline rviz_common::properties::Property * findChild(
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

inline std::vector<QString> childNames(rviz_common::properties::Property * parent)
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

inline std::vector<QString> completionsFor(QCompleter * completer, const QString & prefix)
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

inline std::string plotDisplaySource()
{
  const std::filesystem::path test_file{__FILE__};
  const std::filesystem::path source_file =
    test_file.parent_path().parent_path() / "src" / "plot_2d_display.cpp";
  std::ifstream input(source_file);
  return std::string(
    std::istreambuf_iterator<char>(input),
    std::istreambuf_iterator<char>());
}

inline std::string functionBody(
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

}  // namespace rviz_2d_plot_plugin::test

#endif  // PLOT_2D_DISPLAY_TEST_HELPERS_HPP_
