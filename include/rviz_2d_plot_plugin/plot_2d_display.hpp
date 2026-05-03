// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rviz_common/display.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"
#include "rviz_2d_plot_plugin/plot_2d_renderer.hpp"
#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

namespace rviz_common
{
namespace properties
{
class BoolProperty;
class ColorProperty;
class EditableEnumProperty;
class FloatProperty;
class IntProperty;
class Property;
class StringProperty;
}  // namespace properties
}  // namespace rviz_common

namespace rviz_2d_overlay_plugins
{
class OverlayObject;
}  // namespace rviz_2d_overlay_plugins

namespace rviz_2d_plot_plugin
{

class Plot2DDisplayTestAccessor;

class Plot2DDisplay : public rviz_common::Display
{
  Q_OBJECT

  friend class Plot2DDisplayTestAccessor;

public:
  Plot2DDisplay();
  ~Plot2DDisplay() override;

protected:
  void onInitialize() override;
  void onEnable() override;
  void onDisable() override;
  void update(float wall_dt, float ros_dt) override;
  void reset() override;

private Q_SLOTS:
  void onConfigPropertyChanged();
  void onSeriesCountChanged();
  void onClearHistoryChanged();
  void onTopicOptionsRequested(
    rviz_common::properties::EditableEnumProperty * property);
  void onFieldOptionsRequested(
    rviz_common::properties::EditableEnumProperty * property);

private:
  using SerializedMessageCallback =
    std::function<void (std::shared_ptr<rclcpp::SerializedMessage>)>;

  struct RosGraphOps
  {
    std::function<TopicTypeMap()> get_topic_names_and_types;
  };

  struct SubscriptionFactory
  {
    std::function<rclcpp::GenericSubscription::SharedPtr(
        const std::string &,
        const std::string &,
        rclcpp::QoS,
        SerializedMessageCallback)> create_generic_subscription;
  };

  struct SeriesPropertySet
  {
    rviz_common::properties::Property * root{nullptr};
    rviz_common::properties::BoolProperty * enabled{nullptr};
    rviz_common::properties::EditableEnumProperty * topic{nullptr};
    rviz_common::properties::EditableEnumProperty * field{nullptr};
    rviz_common::properties::StringProperty * label{nullptr};
  };

  std::vector<SeriesConfig> seriesConfigFromProperties_() const;
  Plot2DConfig configFromProperties_() const;
  void rebuildSeriesProperties_(
    int count,
    const std::vector<SeriesConfig> & values);
  const SeriesPropertySet * seriesPropertiesForField_(
    rviz_common::properties::EditableEnumProperty * property) const;
  void resolveAndSubscribe_();
  void onSerializedMessage_(
    const std::string & topic,
    std::shared_ptr<rclcpp::SerializedMessage> message);
  void updateStatusFromController_();
  PlotRenderSettings renderSettingsFromProperties_() const;
  std::vector<RenderableSeries> renderableSeries_() const;
  void updateOverlayGeometry_();
  void renderOverlay_();
  void unsubscribe_();
  double receiveNowSeconds_() const;
  std::vector<std::string> topicOptions_() const;
  std::vector<std::string> fieldOptionsForTopic_(const std::string & topic) const;

  rviz_common::properties::BoolProperty * pause_plot_property_{nullptr};
  rviz_common::properties::BoolProperty * clear_history_property_{nullptr};
  rviz_common::properties::Property * series_root_property_{nullptr};
  rviz_common::properties::IntProperty * series_count_property_{nullptr};
  std::vector<SeriesPropertySet> series_properties_;
  rviz_common::properties::Property * time_root_property_{nullptr};
  rviz_common::properties::FloatProperty * window_seconds_property_{nullptr};
  rviz_common::properties::FloatProperty * refresh_rate_property_{nullptr};
  rviz_common::properties::Property * y_axis_root_property_{nullptr};
  rviz_common::properties::BoolProperty * auto_scale_property_{nullptr};
  rviz_common::properties::FloatProperty * y_min_property_{nullptr};
  rviz_common::properties::FloatProperty * y_max_property_{nullptr};
  rviz_common::properties::Property * layout_root_property_{nullptr};
  rviz_common::properties::IntProperty * width_property_{nullptr};
  rviz_common::properties::IntProperty * height_property_{nullptr};
  rviz_common::properties::IntProperty * x_offset_property_{nullptr};
  rviz_common::properties::IntProperty * y_offset_property_{nullptr};
  rviz_common::properties::Property * style_root_property_{nullptr};
  rviz_common::properties::ColorProperty * background_color_property_{nullptr};
  rviz_common::properties::ColorProperty * axis_color_property_{nullptr};
  rviz_common::properties::ColorProperty * grid_color_property_{nullptr};
  rviz_common::properties::ColorProperty * text_color_property_{nullptr};

  std::shared_ptr<rviz_2d_overlay_plugins::OverlayObject> overlay_;
  rclcpp::Node::SharedPtr node_;
  std::vector<rclcpp::GenericSubscription::SharedPtr> subscriptions_;
  rclcpp::QoS qos_profile_{10};
  RosGraphOps ros_graph_ops_;
  SubscriptionFactory subscription_factory_;
  Plot2DController controller_;
  Plot2DRenderer renderer_;
  double retry_elapsed_seconds_{0.0};
  double render_elapsed_seconds_{0.0};
  mutable std::mutex controller_mutex_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
