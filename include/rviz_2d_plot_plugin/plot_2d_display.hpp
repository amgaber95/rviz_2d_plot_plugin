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
class Config;

namespace properties
{
class BoolProperty;
class ColorProperty;
class EditableEnumProperty;
class EnumProperty;
class FloatProperty;
class IntProperty;
class Property;
class StringProperty;
}  // namespace properties
}  // namespace rviz_common

namespace Ogre
{
class SceneManager;
}  // namespace Ogre

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
  void load(const rviz_common::Config & config) override;

protected:
  void onInitialize() override;
  void onEnable() override;
  void onDisable() override;
  void update(float wall_dt, float ros_dt) override;
  void reset() override;

private Q_SLOTS:
  void onConfigPropertyChanged();
  void onRenderPropertyChanged();
  void onPlotModeChanged();
  void onSeriesCountChanged();
  void onSeriesActionChanged();
  void onApplyReferencePresetChanged();
  void onReferenceCountChanged();
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

  struct OverlayBackendOps
  {
    std::function<void(Ogre::SceneManager *)> prepare_overlays;
  };

  struct SeriesPropertySet
  {
    rviz_common::properties::Property * root{nullptr};
    rviz_common::properties::EnumProperty * action{nullptr};
    rviz_common::properties::BoolProperty * enabled{nullptr};
    rviz_common::properties::EditableEnumProperty * topic{nullptr};
    rviz_common::properties::EditableEnumProperty * x_field{nullptr};
    rviz_common::properties::EditableEnumProperty * y_field{nullptr};
    rviz_common::properties::EditableEnumProperty * field{nullptr};
    rviz_common::properties::StringProperty * label{nullptr};
    rviz_common::properties::StringProperty * unit{nullptr};
    rviz_common::properties::ColorProperty * color{nullptr};
    rviz_common::properties::FloatProperty * line_width{nullptr};
    rviz_common::properties::FloatProperty * line_alpha{nullptr};
    rviz_common::properties::EnumProperty * line_style{nullptr};
    rviz_common::properties::EnumProperty * plot_style{nullptr};
    rviz_common::properties::FloatProperty * value_scale{nullptr};
    rviz_common::properties::FloatProperty * value_offset{nullptr};
  };

  struct ReferencePropertySet
  {
    rviz_common::properties::Property * root{nullptr};
    rviz_common::properties::BoolProperty * enabled{nullptr};
    rviz_common::properties::FloatProperty * value{nullptr};
    rviz_common::properties::FloatProperty * tolerance{nullptr};
    rviz_common::properties::StringProperty * label{nullptr};
    rviz_common::properties::ColorProperty * color{nullptr};
    rviz_common::properties::FloatProperty * alpha{nullptr};
    rviz_common::properties::FloatProperty * line_width{nullptr};
    rviz_common::properties::EnumProperty * line_style{nullptr};
  };

  struct RenderSnapshot
  {
    Plot2DConfig config;
    Plot2DControllerState controller_state;
  };

  std::vector<SeriesConfig> seriesConfigFromProperties_() const;
  std::vector<ReferenceConfig> referenceConfigFromProperties_() const;
  Plot2DConfig configFromProperties_() const;
  void rebuildSeriesProperties_(
    int count,
    const std::vector<SeriesConfig> & values);
  void replaceSeriesProperties_(const std::vector<SeriesConfig> & values);
  void rebuildReferenceProperties_(
    int count,
    const std::vector<ReferenceConfig> & values);
  void appendReferencePreset_();
  void updateModePropertyVisibility_();
  void updateSeriesPropertySummaries_();
  const SeriesPropertySet * seriesPropertiesForField_(
    rviz_common::properties::EditableEnumProperty * property) const;
  void resolveAndSubscribe_();
  void onSerializedMessage_(
    const std::string & topic,
    std::shared_ptr<rclcpp::SerializedMessage> message);
  void updateStatusFromController_();
  void updateStatusFromController_(const Plot2DControllerState & state);
  RenderSnapshot renderSnapshot_() const;
  PlotRenderSettings renderSettingsFromProperties_() const;
  PlotRenderSettings renderSettingsFromConfig_(const Plot2DConfig & config) const;
  std::vector<RenderableSeries> renderableSeries_() const;
  std::vector<RenderableSeries> renderableSeriesFromSnapshot_(
    const RenderSnapshot & snapshot) const;
  std::vector<RenderableReference> renderableReferences_() const;
  std::vector<RenderableReference> renderableReferencesFromConfig_(
    const Plot2DConfig & config) const;
  void updateOverlayGeometry_();
  void updateOverlayGeometry_(const Plot2DConfig & config);
  void renderOverlay_();
  void unsubscribe_();
  bool shouldRetrySubscriptions_() const;
  double receiveNowSeconds_() const;
  double plotNowSeconds_(TimeSource source) const;
  void prepareOverlayRendering_();
  TopicTypeMap topicNamesAndTypes_() const;
  std::vector<std::string> topicOptions_() const;
  std::vector<std::string> fieldOptionsForTopic_(const std::string & topic) const;

  rviz_common::properties::BoolProperty * pause_plot_property_{nullptr};
  rviz_common::properties::BoolProperty * clear_history_property_{nullptr};
  rviz_common::properties::EnumProperty * plot_mode_property_{nullptr};
  rviz_common::properties::Property * series_root_property_{nullptr};
  rviz_common::properties::IntProperty * series_count_property_{nullptr};
  std::vector<SeriesPropertySet> series_properties_;
  rviz_common::properties::Property * time_root_property_{nullptr};
  rviz_common::properties::EnumProperty * time_source_property_{nullptr};
  rviz_common::properties::FloatProperty * window_seconds_property_{nullptr};
  rviz_common::properties::FloatProperty * refresh_rate_property_{nullptr};
  rviz_common::properties::Property * x_axis_root_property_{nullptr};
  rviz_common::properties::EnumProperty * xy_history_mode_property_{nullptr};
  rviz_common::properties::BoolProperty * x_auto_scale_property_{nullptr};
  rviz_common::properties::FloatProperty * x_min_property_{nullptr};
  rviz_common::properties::FloatProperty * x_max_property_{nullptr};
  rviz_common::properties::EnumProperty * x_axis_scale_property_{nullptr};
  rviz_common::properties::Property * y_axis_root_property_{nullptr};
  rviz_common::properties::BoolProperty * auto_scale_property_{nullptr};
  rviz_common::properties::FloatProperty * y_min_property_{nullptr};
  rviz_common::properties::FloatProperty * y_max_property_{nullptr};
  rviz_common::properties::Property * grid_root_property_{nullptr};
  rviz_common::properties::BoolProperty * show_major_grid_property_{nullptr};
  rviz_common::properties::BoolProperty * show_minor_grid_property_{nullptr};
  rviz_common::properties::IntProperty * x_major_tick_count_property_{nullptr};
  rviz_common::properties::IntProperty * y_major_tick_count_property_{nullptr};
  rviz_common::properties::IntProperty * minor_grid_divisions_property_{nullptr};
  rviz_common::properties::Property * references_root_property_{nullptr};
  rviz_common::properties::EnumProperty * reference_preset_property_{nullptr};
  rviz_common::properties::FloatProperty * reference_preset_value_property_{nullptr};
  rviz_common::properties::FloatProperty * reference_preset_tolerance_property_{nullptr};
  rviz_common::properties::BoolProperty * apply_reference_preset_property_{nullptr};
  rviz_common::properties::IntProperty * reference_count_property_{nullptr};
  std::vector<ReferencePropertySet> reference_properties_;
  rviz_common::properties::Property * legend_root_property_{nullptr};
  rviz_common::properties::BoolProperty * show_legend_property_{nullptr};
  rviz_common::properties::BoolProperty * show_latest_values_property_{nullptr};
  rviz_common::properties::EnumProperty * legend_position_property_{nullptr};
  rviz_common::properties::IntProperty * legend_x_offset_property_{nullptr};
  rviz_common::properties::IntProperty * legend_y_offset_property_{nullptr};
  rviz_common::properties::Property * layout_root_property_{nullptr};
  rviz_common::properties::IntProperty * width_property_{nullptr};
  rviz_common::properties::IntProperty * height_property_{nullptr};
  rviz_common::properties::IntProperty * x_offset_property_{nullptr};
  rviz_common::properties::IntProperty * y_offset_property_{nullptr};
  rviz_common::properties::EnumProperty * horizontal_alignment_property_{nullptr};
  rviz_common::properties::EnumProperty * vertical_alignment_property_{nullptr};
  rviz_common::properties::Property * style_root_property_{nullptr};
  rviz_common::properties::ColorProperty * background_color_property_{nullptr};
  rviz_common::properties::FloatProperty * background_alpha_property_{nullptr};
  rviz_common::properties::ColorProperty * axis_color_property_{nullptr};
  rviz_common::properties::ColorProperty * grid_color_property_{nullptr};
  rviz_common::properties::ColorProperty * text_color_property_{nullptr};
  rviz_common::properties::IntProperty * font_size_property_{nullptr};

  std::shared_ptr<rviz_2d_overlay_plugins::OverlayObject> overlay_;
  rclcpp::Node::SharedPtr node_;
  std::vector<rclcpp::GenericSubscription::SharedPtr> subscriptions_;
  rclcpp::QoS qos_profile_{10};
  RosGraphOps ros_graph_ops_;
  SubscriptionFactory subscription_factory_;
  OverlayBackendOps overlay_backend_ops_;
  Plot2DController controller_;
  Plot2DRenderer renderer_;
  double retry_elapsed_seconds_{0.0};
  double render_elapsed_seconds_{0.0};
  mutable std::mutex controller_mutex_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
