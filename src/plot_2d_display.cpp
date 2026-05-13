// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_display.hpp"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QSignalBlocker>
#include <QTimer>
#include <QVariant>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <sstream>
#include <utility>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/config.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/enum_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/property.hpp>
#include <rviz_common/properties/status_property.hpp>
#include <rviz_common/properties/string_property.hpp>

#include "overlay_backend.hpp"
#include "plot_2d_display_options.hpp"
#include "plot_2d_property_helpers.hpp"
#include "plot_2d_subscription_manager.hpp"

namespace rviz_2d_plot_plugin
{
namespace
{

rviz_common::properties::StatusProperty::Level statusLevel(
  const PlotControllerStatus status)
{
  using rviz_common::properties::StatusProperty;
  switch (status) {
    case PlotControllerStatus::Ok:
      return StatusProperty::Ok;
    case PlotControllerStatus::Disabled:
    case PlotControllerStatus::EmptySelection:
    case PlotControllerStatus::WaitingForTopic:
      return StatusProperty::Warn;
    case PlotControllerStatus::InvalidPath:
    case PlotControllerStatus::UnsupportedPathSyntax:
    case PlotControllerStatus::MissingFieldPath:
    case PlotControllerStatus::AmbiguousTopicType:
    case PlotControllerStatus::ExtractorError:
    case PlotControllerStatus::ExtractionError:
      return StatusProperty::Error;
  }
  return StatusProperty::Error;
}

std::string statusText(const Plot2DControllerState & state)
{
  if (!state.message.empty()) {
    return state.message;
  }
  if (state.status == PlotControllerStatus::Ok) {
    return "OK";
  }
  return "Waiting for a topic and field selection";
}

}  // namespace

Plot2DDisplay::Plot2DDisplay()
{
  subscription_manager_ = std::make_unique<Plot2DSubscriptionManager>();
  overlay_backend_factory_ =
    [](std::string name)
    {
      return makeOgreOverlayBackend(std::move(name));
    };

  pause_plot_property_ = new rviz_common::properties::BoolProperty(
    "Pause Plot", false, "Pause incoming sample collection and hold the plot.",
    this, SLOT(onConfigPropertyChanged()), this);
  clear_history_property_ = new rviz_common::properties::BoolProperty(
    "Clear History", false, "Clear stored samples for this plot.",
    this, SLOT(onClearHistoryChanged()), this);

  plot_mode_property_ = new rviz_common::properties::EnumProperty(
    "Plot Mode", QString::fromStdString(plotModeName(PlotMode::TimeSeries)),
    "Choose time-series or same-topic XY plotting.",
    this, SLOT(onPlotModeChanged()), this);
  addPlotModeOptions(plot_mode_property_);

  auto * series_root = new ReorderableListProperty(
    "Series", QVariant(), "Topic field series to draw.", this);
  series_root->setFixedChildCount(1);
  series_root_property_ = series_root;
  QObject::connect(
    series_root_property_,
    &rviz_common::properties::Property::childListChanged,
    this,
    &Plot2DDisplay::onSeriesChildListChanged_);
  series_count_property_ = new rviz_common::properties::IntProperty(
    "Series Count", 1, "Number of plotted topic fields.",
    series_root_property_, SLOT(onSeriesCountChanged()), this, 1, 12);
  rebuildSeriesProperties_(1, {SeriesConfig{}});

  time_root_property_ = new rviz_common::properties::Property(
    "Time", QVariant(), "Time-series history and redraw settings.", this);
  time_source_property_ = new rviz_common::properties::EnumProperty(
    "Time Source", QString::fromStdString(timeSourceName(TimeSource::ReceiveTime)),
    "Timestamp samples by receive time or by message header stamp when available.",
    time_root_property_, SLOT(onConfigPropertyChanged()), this);
  addTimeSourceOptions(time_source_property_);
  window_seconds_property_ = new rviz_common::properties::FloatProperty(
    "Window Seconds", 30.0F, "Visible rolling time window in seconds.",
    time_root_property_, SLOT(onConfigPropertyChanged()), this);
  window_seconds_property_->setMin(1.0F);
  xy_history_mode_property_ = new rviz_common::properties::EnumProperty(
    "XY History Mode",
    QString::fromStdString(xyHistoryModeName(XYHistoryMode::RollingTimeWindow)),
    "How XY samples are retained.",
    time_root_property_, SLOT(onConfigPropertyChanged()), this);
  addXYHistoryModeOptions(xy_history_mode_property_);
  refresh_rate_property_ = new rviz_common::properties::FloatProperty(
    "Refresh Rate", 20.0F, "Overlay redraw rate in Hz.", time_root_property_,
    SLOT(onRenderPropertyChanged()), this);
  refresh_rate_property_->setMin(1.0F);

  qos_root_property_ = new rviz_common::properties::Property(
    "QoS", QVariant(), "ROS subscription quality-of-service settings.", this);
  qos_reliability_property_ = new rviz_common::properties::EnumProperty(
    "Reliability", QString::fromStdString(qosReliabilityName(QoSReliability::Reliable)),
    "Subscription reliability policy.",
    qos_root_property_, SLOT(onConfigPropertyChanged()), this);
  addQoSReliabilityOptions(qos_reliability_property_);
  qos_durability_property_ = new rviz_common::properties::EnumProperty(
    "Durability", QString::fromStdString(qosDurabilityName(QoSDurability::Volatile)),
    "Subscription durability policy.",
    qos_root_property_, SLOT(onConfigPropertyChanged()), this);
  addQoSDurabilityOptions(qos_durability_property_);
  qos_depth_property_ = new rviz_common::properties::IntProperty(
    "Depth", 10, "Keep-last queue depth for subscriptions.",
    qos_root_property_, SLOT(onConfigPropertyChanged()), this, 1, 100000);

  x_axis_root_property_ = new rviz_common::properties::Property(
    "X Axis", QVariant(), "X-axis scale settings for XY mode.", this);
  x_auto_scale_property_ = new rviz_common::properties::BoolProperty(
    "Auto Scale", true, "Automatically fit XY x-axis values.",
    x_axis_root_property_, SLOT(onRenderPropertyChanged()), this);
  x_min_property_ = new rviz_common::properties::FloatProperty(
    "X Min", -1.0F, "Fixed x-axis minimum when auto scale is disabled.",
    x_axis_root_property_, SLOT(onRenderPropertyChanged()), this);
  x_max_property_ = new rviz_common::properties::FloatProperty(
    "X Max", 1.0F, "Fixed x-axis maximum when auto scale is disabled.",
    x_axis_root_property_, SLOT(onRenderPropertyChanged()), this);
  x_axis_scale_property_ = new rviz_common::properties::EnumProperty(
    "Axis Scale",
    QString::fromStdString(xyAxisScaleModeName(XYAxisScaleMode::Independent)),
    "XY axis scale relationship.", x_axis_root_property_,
    SLOT(onRenderPropertyChanged()), this);
  addXYAxisScaleModeOptions(x_axis_scale_property_);

  y_axis_root_property_ = new rviz_common::properties::Property(
    "Y Axis", QVariant(), "Vertical value axis scaling.", this);
  auto_scale_property_ = new rviz_common::properties::BoolProperty(
    "Auto Scale", true, "Automatically fit the y-axis to visible samples.",
    y_axis_root_property_, SLOT(onRenderPropertyChanged()), this);
  y_min_property_ = new rviz_common::properties::FloatProperty(
    "Y Min", -1.0F, "Fixed y-axis minimum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onRenderPropertyChanged()), this);
  y_max_property_ = new rviz_common::properties::FloatProperty(
    "Y Max", 1.0F, "Fixed y-axis maximum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onRenderPropertyChanged()), this);

  grid_root_property_ = new rviz_common::properties::Property(
    "Grid", QVariant(), "Plot grid density and visibility.", this);
  show_major_grid_property_ = new rviz_common::properties::BoolProperty(
    "Major Grid", true, "Draw major grid lines.",
    grid_root_property_, SLOT(onRenderPropertyChanged()), this);
  show_minor_grid_property_ = new rviz_common::properties::BoolProperty(
    "Minor Grid", true, "Draw subtle minor grid lines between major ticks.",
    grid_root_property_, SLOT(onRenderPropertyChanged()), this);
  x_major_tick_count_property_ = new rviz_common::properties::IntProperty(
    "X Major Ticks", 6, "Target number of major ticks on the x-axis.",
    grid_root_property_, SLOT(onRenderPropertyChanged()), this, 2, 20);
  y_major_tick_count_property_ = new rviz_common::properties::IntProperty(
    "Y Major Ticks", 5, "Target number of major ticks on the y-axis.",
    grid_root_property_, SLOT(onRenderPropertyChanged()), this, 2, 20);
  minor_grid_divisions_property_ = new rviz_common::properties::IntProperty(
    "Minor Divisions", 1, "Minor grid lines per major tick interval.",
    grid_root_property_, SLOT(onRenderPropertyChanged()), this, 0, 8);

  auto * references_root = new ReorderableListProperty(
    "References", QVariant(), "Horizontal reference lines.", this);
  references_root->setFixedChildCount(kReferenceFixedPropertyCount);
  references_root_property_ = references_root;
  QObject::connect(
    references_root_property_,
    &rviz_common::properties::Property::childListChanged,
    this,
    &Plot2DDisplay::onReferenceChildListChanged_);
  reference_preset_property_ = new rviz_common::properties::EnumProperty(
    "Preset", kNoReferencePreset, "Common reference line presets.",
    references_root_property_);
  addReferencePresetOptions(reference_preset_property_);
  reference_preset_value_property_ = new rviz_common::properties::FloatProperty(
    "Preset Value", 0.0F,
    "Y-axis value used by the selected preset.",
    references_root_property_);
  reference_preset_tolerance_property_ = new rviz_common::properties::FloatProperty(
    "Preset Tolerance", 0.1F,
    "Half-width used by the tolerance band preset.",
    references_root_property_);
  reference_preset_tolerance_property_->setMin(0.0F);
  apply_reference_preset_property_ = new rviz_common::properties::BoolProperty(
    "Apply Preset", false, "Append the selected preset to the reference list.",
    references_root_property_, SLOT(onApplyReferencePresetChanged()), this);
  apply_reference_preset_property_->setShouldBeSaved(false);
  reference_count_property_ = new rviz_common::properties::IntProperty(
    "Reference Count", 0, "Number of horizontal reference lines.",
    references_root_property_, SLOT(onReferenceCountChanged()), this, 0, 12);
  rebuildReferenceProperties_(0, {});

  legend_root_property_ = new rviz_common::properties::Property(
    "Legend", QVariant(), "Legend display and placement.", this);
  show_legend_property_ = new rviz_common::properties::BoolProperty(
    "Enabled", true, "Show the legend inside the plot area.",
    legend_root_property_, SLOT(onRenderPropertyChanged()), this);
  show_latest_values_property_ = new rviz_common::properties::BoolProperty(
    "Show Values", true, "Show latest visible sample values next to legend labels.",
    legend_root_property_, SLOT(onRenderPropertyChanged()), this);
  legend_position_property_ = new rviz_common::properties::EnumProperty(
    "Position", QString::fromStdString(legendPositionName(LegendPosition::TopLeft)),
    "Legend placement inside the plot area.",
    legend_root_property_, SLOT(onRenderPropertyChanged()), this);
  addLegendPositionOptions(legend_position_property_);
  legend_x_offset_property_ = new rviz_common::properties::IntProperty(
    "X Offset", 4, "Horizontal legend inset in pixels.",
    legend_root_property_, SLOT(onRenderPropertyChanged()), this);
  legend_x_offset_property_->setMin(0);
  legend_y_offset_property_ = new rviz_common::properties::IntProperty(
    "Y Offset", 4, "Vertical legend inset in pixels.",
    legend_root_property_, SLOT(onRenderPropertyChanged()), this);
  legend_y_offset_property_->setMin(0);

  layout_root_property_ = new rviz_common::properties::Property(
    "Layout", QVariant(), "Overlay size and screen position.", this);
  width_property_ = new rviz_common::properties::IntProperty(
    "Width", 360, "Overlay width in pixels.", layout_root_property_,
    SLOT(onRenderPropertyChanged()), this, 120);
  height_property_ = new rviz_common::properties::IntProperty(
    "Height", 220, "Overlay height in pixels.", layout_root_property_,
    SLOT(onRenderPropertyChanged()), this, 80);
  x_offset_property_ = new rviz_common::properties::IntProperty(
    "X Offset", 10, "Horizontal screen offset in pixels.", layout_root_property_,
    SLOT(onRenderPropertyChanged()), this);
  y_offset_property_ = new rviz_common::properties::IntProperty(
    "Y Offset", 10, "Vertical screen offset in pixels.", layout_root_property_,
    SLOT(onRenderPropertyChanged()), this);
  horizontal_alignment_property_ = new rviz_common::properties::EnumProperty(
    "Horizontal Alignment",
    QString::fromStdString(horizontalAlignmentName(HorizontalAlignment::Right)),
    "Horizontal screen anchor used by X Offset.",
    layout_root_property_, SLOT(onRenderPropertyChanged()), this);
  addHorizontalAlignmentOptions(horizontal_alignment_property_);
  vertical_alignment_property_ = new rviz_common::properties::EnumProperty(
    "Vertical Alignment",
    QString::fromStdString(verticalAlignmentName(VerticalAlignment::Top)),
    "Vertical screen anchor used by Y Offset.",
    layout_root_property_, SLOT(onRenderPropertyChanged()), this);
  addVerticalAlignmentOptions(vertical_alignment_property_);

  style_root_property_ = new rviz_common::properties::Property(
    "Style", QVariant(), "Plot colors.", this);
  background_color_property_ = new rviz_common::properties::ColorProperty(
    "Background Color", QColor(0, 0, 0), "Plot background color.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this);
  background_alpha_property_ = new rviz_common::properties::FloatProperty(
    "Background Alpha", 190.0F / 255.0F, "Plot background opacity from 0 to 1.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this);
  background_alpha_property_->setMin(0.0F);
  background_alpha_property_->setMax(1.0F);
  axis_color_property_ = new rviz_common::properties::ColorProperty(
    "Axis Color", QColor(230, 230, 230), "Axis and border color.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this);
  grid_color_property_ = new rviz_common::properties::ColorProperty(
    "Grid Color", QColor(130, 130, 130), "Grid line color.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this);
  text_color_property_ = new rviz_common::properties::ColorProperty(
    "Text Color", QColor(245, 245, 245), "Axis and legend text color.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this);
  font_size_property_ = new rviz_common::properties::IntProperty(
    "Font Size", 8, "Axis, legend, and reference label font size in points.",
    style_root_property_, SLOT(onRenderPropertyChanged()), this, 6, 16);
  updateModePropertyVisibility_();
}

Plot2DDisplay::~Plot2DDisplay() = default;

void Plot2DDisplay::load(const rviz_common::Config & config)
{
  int count = 0;
  const rviz_common::Config series_config = config.mapGetChild("Series");
  if (series_config.mapGetInt("Series Count", &count)) {
    rebuildSeriesProperties_(count, seriesConfigFromProperties_());
  }

  const rviz_common::Config references_config = config.mapGetChild("References");
  if (references_config.mapGetInt("Reference Count", &count)) {
    rebuildReferenceProperties_(count, referenceConfigFromProperties_());
  }

  rviz_common::Display::load(config);
  updateModePropertyVisibility_();
  updateSeriesPropertySummaries_();
  updateReferencePropertySummaries_();
}

void Plot2DDisplay::onInitialize()
{
  rviz_common::Display::onInitialize();
  if (context_) {
    auto rviz_node = context_->getRosNodeAbstraction().lock();
    if (rviz_node) {
      node_ = rviz_node->get_raw_node();
    }
  }

  if (node_) {
    ros_graph_ops_.get_topic_names_and_types = [this]() {
        const auto context = node_->get_node_base_interface()->get_context();
        if (!context || !context->is_valid()) {
          return TopicTypeMap{};
        }
        return node_->get_topic_names_and_types();
      };
    subscription_manager_->setFactory(
      [this](
        const std::string & topic,
        const std::string & type,
        rclcpp::QoS qos,
        PlotSerializedMessageCallback callback)
      {
        return node_->create_generic_subscription(
          topic, type, qos, std::move(callback));
      });
  }

  initializeOverlayBackend_();
  resolveAndSubscribe_();
}

void Plot2DDisplay::onEnable()
{
  resolveAndSubscribe_();
  if (overlay_backend_) {
    overlay_backend_->setVisible(true);
  }
  renderOverlay_();
}

void Plot2DDisplay::onDisable()
{
  unsubscribe_();
  if (overlay_backend_) {
    overlay_backend_->setVisible(false);
  }
}

void Plot2DDisplay::update(const float wall_dt, const float ros_dt)
{
  (void)ros_dt;
  retry_elapsed_seconds_ += std::max(0.0F, wall_dt);
  if (retry_elapsed_seconds_ >= 1.0) {
    retry_elapsed_seconds_ = 0.0;
    if (shouldRetrySubscriptions_()) {
      resolveAndSubscribe_();
    }
  }

  render_elapsed_seconds_ += std::max(0.0F, wall_dt);
  const double refresh_rate = std::max(1.0F, refresh_rate_property_->getFloat());
  if (render_elapsed_seconds_ >= 1.0 / refresh_rate) {
    render_elapsed_seconds_ = 0.0;
    renderOverlay_(false);
  }
}

void Plot2DDisplay::reset()
{
  rviz_common::Display::reset();
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    controller_.clearHistory();
  }
  renderOverlay_();
}

void Plot2DDisplay::onConfigPropertyChanged()
{
  updateSeriesPropertySummaries_();
  updateReferencePropertySummaries_();
  resolveAndSubscribe_();
  renderOverlay_();
}

void Plot2DDisplay::onRenderPropertyChanged()
{
  renderOverlay_();
}

void Plot2DDisplay::onSeriesAppearancePropertyChanged()
{
  updateSeriesPropertySummaries_();
  renderOverlay_();
}

void Plot2DDisplay::onReferencePropertyChanged()
{
  updateReferencePropertySummaries_();
  renderOverlay_();
}

void Plot2DDisplay::onPlotModeChanged()
{
  updateModePropertyVisibility_();
  onConfigPropertyChanged();
}

void Plot2DDisplay::onSeriesCountChanged()
{
  const std::vector<SeriesConfig> current = seriesConfigFromProperties_();
  rebuildSeriesProperties_(series_count_property_->getInt(), current);
  onConfigPropertyChanged();
}

void Plot2DDisplay::onDuplicateSeriesChanged()
{
  auto * duplicate_property =
    qobject_cast<rviz_common::properties::BoolProperty *>(sender());
  if (!duplicate_property || !duplicate_property->getBool()) {
    return;
  }

  const auto property_it = std::find_if(
    series_properties_.begin(), series_properties_.end(),
    [duplicate_property](const SeriesPropertySet & properties) {
      return properties.duplicate == duplicate_property;
    });
  if (property_it == series_properties_.end()) {
    return;
  }

  std::vector<SeriesConfig> series = seriesConfigFromProperties_();
  const std::size_t index = static_cast<std::size_t>(
    std::distance(series_properties_.begin(), property_it));

  if (index >= series.size() || series.size() >= 12U) {
    const QSignalBlocker blocker(duplicate_property);
    duplicate_property->setBool(false);
    return;
  }

  SeriesConfig copy = series[index];
  copy.label = copy.label.empty() ? "Series Copy" : copy.label + " Copy";
  series.insert(series.begin() + static_cast<std::ptrdiff_t>(index + 1), copy);

  QTimer::singleShot(
    0,
    this,
    [this, series = std::move(series)]() mutable {
      replaceSeriesProperties_(series);
      onConfigPropertyChanged();
    });
}

void Plot2DDisplay::onDeleteSeriesChanged()
{
  auto * delete_property =
    qobject_cast<rviz_common::properties::BoolProperty *>(sender());
  if (!delete_property || !delete_property->getBool()) {
    return;
  }

  const auto property_it = std::find_if(
    series_properties_.begin(), series_properties_.end(),
    [delete_property](const SeriesPropertySet & properties) {
      return properties.delete_series == delete_property;
    });
  if (property_it == series_properties_.end()) {
    return;
  }

  std::vector<SeriesConfig> series = seriesConfigFromProperties_();
  const std::size_t index = static_cast<std::size_t>(
    std::distance(series_properties_.begin(), property_it));
  if (index >= series.size() || series.size() <= 1U) {
    const QSignalBlocker blocker(delete_property);
    delete_property->setBool(false);
    return;
  }

  series.erase(series.begin() + static_cast<std::ptrdiff_t>(index));

  QTimer::singleShot(
    0,
    this,
    [this, series = std::move(series)]() mutable {
      replaceSeriesProperties_(series);
      onConfigPropertyChanged();
    });
}

void Plot2DDisplay::onApplyReferencePresetChanged()
{
  if (!apply_reference_preset_property_ || !apply_reference_preset_property_->getBool()) {
    return;
  }

  appendReferencePreset_();
  const QSignalBlocker blocker(apply_reference_preset_property_);
  apply_reference_preset_property_->setBool(false);
}

void Plot2DDisplay::onReferenceCountChanged()
{
  const std::vector<ReferenceConfig> current = referenceConfigFromProperties_();
  rebuildReferenceProperties_(reference_count_property_->getInt(), current);
  onReferencePropertyChanged();
}

void Plot2DDisplay::onDuplicateReferenceChanged()
{
  auto * duplicate_property =
    qobject_cast<rviz_common::properties::BoolProperty *>(sender());
  if (!duplicate_property || !duplicate_property->getBool()) {
    return;
  }

  const auto property_it = std::find_if(
    reference_properties_.begin(), reference_properties_.end(),
    [duplicate_property](const ReferencePropertySet & properties) {
      return properties.duplicate == duplicate_property;
    });
  if (property_it == reference_properties_.end()) {
    return;
  }

  std::vector<ReferenceConfig> references = referenceConfigFromProperties_();
  const std::size_t index = static_cast<std::size_t>(
    std::distance(reference_properties_.begin(), property_it));
  if (index >= references.size() || references.size() >= 12U) {
    const QSignalBlocker blocker(duplicate_property);
    duplicate_property->setBool(false);
    return;
  }

  ReferenceConfig copy = references[index];
  if (!copy.label.empty()) {
    copy.label += " Copy";
  }
  references.insert(references.begin() + static_cast<std::ptrdiff_t>(index + 1), copy);

  QTimer::singleShot(
    0,
    this,
    [this, references = std::move(references)]() mutable {
      replaceReferenceProperties_(references);
      onReferencePropertyChanged();
    });
}

void Plot2DDisplay::onDeleteReferenceChanged()
{
  auto * delete_property =
    qobject_cast<rviz_common::properties::BoolProperty *>(sender());
  if (!delete_property || !delete_property->getBool()) {
    return;
  }

  const auto property_it = std::find_if(
    reference_properties_.begin(), reference_properties_.end(),
    [delete_property](const ReferencePropertySet & properties) {
      return properties.delete_reference == delete_property;
    });
  if (property_it == reference_properties_.end()) {
    return;
  }

  std::vector<ReferenceConfig> references = referenceConfigFromProperties_();
  const std::size_t index = static_cast<std::size_t>(
    std::distance(reference_properties_.begin(), property_it));
  if (index >= references.size()) {
    const QSignalBlocker blocker(delete_property);
    delete_property->setBool(false);
    return;
  }

  references.erase(references.begin() + static_cast<std::ptrdiff_t>(index));

  QTimer::singleShot(
    0,
    this,
    [this, references = std::move(references)]() mutable {
      replaceReferenceProperties_(references);
      onReferencePropertyChanged();
    });
}

void Plot2DDisplay::onClearHistoryChanged()
{
  if (!clear_history_property_ || !clear_history_property_->getBool()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    controller_.clearHistory();
  }
  clear_history_property_->setBool(false);
  renderOverlay_();
}

void Plot2DDisplay::onTopicOptionsRequested(
  rviz_common::properties::EditableEnumProperty * property)
{
  if (!property) {
    return;
  }

  property->clearOptions();
  for (const std::string & topic : topicOptions_()) {
    property->addOptionStd(topic);
  }
}

void Plot2DDisplay::onFieldOptionsRequested(
  rviz_common::properties::EditableEnumProperty * property)
{
  if (!property) {
    return;
  }

  const SeriesPropertySet * series = seriesPropertiesForField_(property);
  if (!series || !series->topic) {
    return;
  }

  property->clearOptions();
  for (const std::string & field :
    fieldOptionsForTopic_(series->topic->getStdString()))
  {
    property->addOptionStd(field);
  }
}

std::vector<SeriesConfig> Plot2DDisplay::seriesConfigFromProperties_() const
{
  std::vector<SeriesConfig> series;
  series.reserve(series_properties_.size());
  for (const SeriesPropertySet & properties : series_properties_) {
    SeriesConfig config;
    config.enabled = properties.root && properties.root->getBool();
    config.topic = properties.topic ? properties.topic->getStdString() : "";
    config.x_field = properties.x_field ? properties.x_field->getStdString() : "";
    config.y_field = properties.y_field ? properties.y_field->getStdString() : "";
    config.field = properties.field ? properties.field->getStdString() : "";
    config.label = properties.label ? properties.label->getStdString() : "Series";
    config.unit = properties.unit ? properties.unit->getStdString() : "";
    config.color = properties.color ? toSeriesColor(properties.color->getColor()) :
      defaultSeriesColor(series.size());
    config.line_width = properties.line_width ? properties.line_width->getFloat() : 2.0;
    config.line_alpha = properties.line_alpha ? properties.line_alpha->getFloat() : 1.0;
    config.line_style = properties.line_style ?
      lineStyleFromName(properties.line_style->getStdString()) : LineStyle::Solid;
    config.plot_style = properties.plot_style ?
      plotStyleFromName(properties.plot_style->getStdString()) : PlotStyle::Line;
    config.value_scale = properties.value_scale ? properties.value_scale->getFloat() : 1.0;
    config.value_offset = properties.value_offset ? properties.value_offset->getFloat() : 0.0;
    series.push_back(std::move(config));
  }
  return series;
}

std::vector<ReferenceConfig> Plot2DDisplay::referenceConfigFromProperties_() const
{
  std::vector<ReferenceConfig> references;
  references.reserve(reference_properties_.size());
  for (const ReferencePropertySet & properties : reference_properties_) {
    ReferenceConfig config;
    config.enabled = properties.root && properties.root->getBool();
    config.value = properties.value ? properties.value->getFloat() : 0.0;
    config.tolerance = properties.tolerance ? properties.tolerance->getFloat() : 0.0;
    config.label = properties.label ? properties.label->getStdString() : "";
    config.color = properties.color ? toSeriesColor(properties.color->getColor()) :
      SeriesColor{255, 180, 60};
    config.alpha = properties.alpha ? properties.alpha->getFloat() : 1.0;
    config.line_width = properties.line_width ? properties.line_width->getFloat() : 1.2;
    config.line_style = properties.line_style ?
      lineStyleFromName(properties.line_style->getStdString()) : LineStyle::Solid;
    references.push_back(std::move(config));
  }
  return references;
}

Plot2DConfig Plot2DDisplay::configFromProperties_() const
{
  Plot2DConfig config;
  config.series = seriesConfigFromProperties_();
  config.references = referenceConfigFromProperties_();
  config.plot_mode = plot_mode_property_ ?
    plotModeFromName(plot_mode_property_->getStdString()) : PlotMode::TimeSeries;

  config.time.window_seconds = window_seconds_property_->getFloat();
  config.time.refresh_rate_hz = refresh_rate_property_->getFloat();
  config.time.paused = pause_plot_property_->getBool();
  config.time.source = time_source_property_ ?
    timeSourceFromName(time_source_property_->getStdString()) : TimeSource::ReceiveTime;
  config.time.xy_history_mode = xy_history_mode_property_ ?
    xyHistoryModeFromName(xy_history_mode_property_->getStdString()) :
    XYHistoryMode::RollingTimeWindow;

  config.qos.reliability = qos_reliability_property_ ?
    qosReliabilityFromName(qos_reliability_property_->getStdString()) :
    QoSReliability::Reliable;
  config.qos.durability = qos_durability_property_ ?
    qosDurabilityFromName(qos_durability_property_->getStdString()) :
    QoSDurability::Volatile;
  config.qos.depth = qos_depth_property_ ? qos_depth_property_->getInt() : 10;

  config.x_axis.mode = config.plot_mode == PlotMode::XY ? XAxisMode::Field : XAxisMode::Time;
  config.x_axis.scale_mode = x_auto_scale_property_->getBool() ?
    AxisScaleMode::Auto : AxisScaleMode::Fixed;
  config.x_axis.axis_scale_mode = x_axis_scale_property_ ?
    xyAxisScaleModeFromName(x_axis_scale_property_->getStdString()) :
    XYAxisScaleMode::Independent;
  config.x_axis.fixed_min = x_min_property_->getFloat();
  config.x_axis.fixed_max = x_max_property_->getFloat();

  config.y_axis.scale_mode = auto_scale_property_->getBool() ?
    AxisScaleMode::Auto : AxisScaleMode::Fixed;
  config.y_axis.fixed_min = y_min_property_->getFloat();
  config.y_axis.fixed_max = y_max_property_->getFloat();

  config.layout.width = width_property_->getInt();
  config.layout.height = height_property_->getInt();
  config.layout.x_offset = x_offset_property_->getInt();
  config.layout.y_offset = y_offset_property_->getInt();
  config.layout.horizontal_alignment = horizontal_alignment_property_ ?
    horizontalAlignmentFromName(horizontal_alignment_property_->getStdString()) :
    HorizontalAlignment::Right;
  config.layout.vertical_alignment = vertical_alignment_property_ ?
    verticalAlignmentFromName(vertical_alignment_property_->getStdString()) :
    VerticalAlignment::Top;
  config.repair();
  return config;
}

void Plot2DDisplay::rebuildSeriesProperties_(
  const int count,
  const std::vector<SeriesConfig> & values)
{
  const int repaired_count = std::clamp(count, 1, 12);
  if (series_count_property_->getInt() != repaired_count) {
    series_count_property_->setInt(repaired_count);
  }

  rebuilding_series_properties_ = true;
  series_root_property_->removeChildren(1);
  series_properties_.clear();
  series_properties_.reserve(static_cast<std::size_t>(repaired_count));

  for (int i = 0; i < repaired_count; ++i) {
    SeriesConfig value;
    if (static_cast<std::size_t>(i) < values.size()) {
      value = values[static_cast<std::size_t>(i)];
      if (value.label == "Series" && value.topic.empty() && value.field.empty() &&
        value.x_field.empty() && value.y_field.empty())
      {
        value.label.clear();
      }
    } else {
      value.label.clear();
      value.color = defaultSeriesColor(static_cast<std::size_t>(i));
    }

    SeriesPropertySet properties;
    const QString name = "Series " + QString::number(i + 1);
    properties.root = new ListItemBoolProperty(
      name, value.enabled, "Enable this plotted topic field.", series_root_property_,
      SLOT(onConfigPropertyChanged()), this);
    properties.duplicate = new rviz_common::properties::BoolProperty(
      "Duplicate", false, "Duplicate this series.",
      properties.root, SLOT(onDuplicateSeriesChanged()), this);
    properties.duplicate->setShouldBeSaved(false);
    properties.delete_series = new rviz_common::properties::BoolProperty(
      "Delete", false, "Delete this series.",
      properties.root, SLOT(onDeleteSeriesChanged()), this);
    properties.delete_series->setShouldBeSaved(false);
    properties.topic = new ContainsFilterEditableEnumProperty(
      "Topic", QString::fromStdString(value.topic), "ROS 2 topic to subscribe to.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.topic,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onTopicOptionsRequested);
    properties.field = new ContainsFilterEditableEnumProperty(
      "Field", QString::fromStdString(value.field),
      "Numeric or boolean field path inside the selected message.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.field,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onFieldOptionsRequested);
    properties.x_field = new ContainsFilterEditableEnumProperty(
      "X Field", QString::fromStdString(value.x_field),
      "Numeric field used for the x-axis in XY mode.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.x_field,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onFieldOptionsRequested);
    properties.y_field = new ContainsFilterEditableEnumProperty(
      "Y Field", QString::fromStdString(value.y_field),
      "Numeric or boolean field used for the y-axis in XY mode.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.y_field,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onFieldOptionsRequested);
    properties.label = new rviz_common::properties::StringProperty(
      "Label", QString::fromStdString(value.label), "Legend label for this series.",
      properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    properties.unit = new rviz_common::properties::StringProperty(
      "Unit", QString::fromStdString(value.unit), "Optional legend unit shown after values.",
      properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    properties.color = new rviz_common::properties::ColorProperty(
      "Color", toQColor(value.color), "Series line color.",
      properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    properties.line_width = new rviz_common::properties::FloatProperty(
      "Line Width", value.line_width, "Series line width in pixels.",
      properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    properties.line_width->setMin(static_cast<float>(kMinimumLineWidth));
    properties.line_width->setMax(static_cast<float>(kMaximumLineWidth));
    properties.line_alpha = new rviz_common::properties::FloatProperty(
      "Line Alpha", value.line_alpha, "Series line opacity from 0 to 1.",
      properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    properties.line_alpha->setMin(0.0F);
    properties.line_alpha->setMax(1.0F);
    properties.line_style = new rviz_common::properties::EnumProperty(
      "Line Style", QString::fromStdString(lineStyleName(value.line_style)),
      "Series line pattern.", properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    addLineStyleOptions(properties.line_style);
    properties.plot_style = new rviz_common::properties::EnumProperty(
      "Plot Style", QString::fromStdString(plotStyleName(value.plot_style)),
      "Series rendering mode.", properties.root, SLOT(onSeriesAppearancePropertyChanged()), this);
    addPlotStyleOptions(properties.plot_style);
    properties.value_scale = new rviz_common::properties::FloatProperty(
      "Value Scale", value.value_scale, "Scale applied to extracted values before plotting.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.value_offset = new rviz_common::properties::FloatProperty(
      "Value Offset", value.value_offset, "Offset added after scaling extracted values.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    series_properties_.push_back(properties);
  }
  rebuilding_series_properties_ = false;
  updateModePropertyVisibility_();
  updateSeriesPropertySummaries_();
}

void Plot2DDisplay::replaceSeriesProperties_(const std::vector<SeriesConfig> & values)
{
  const int count = static_cast<int>(std::clamp<std::size_t>(values.size(), 1U, 12U));
  {
    const QSignalBlocker blocker(series_count_property_);
    series_count_property_->setInt(count);
  }
  rebuildSeriesProperties_(count, values);
}

void Plot2DDisplay::rebuildReferenceProperties_(
  const int count,
  const std::vector<ReferenceConfig> & values)
{
  const int repaired_count = std::clamp(count, 0, 12);
  if (reference_count_property_->getInt() != repaired_count) {
    reference_count_property_->setInt(repaired_count);
  }

  rebuilding_reference_properties_ = true;
  references_root_property_->removeChildren(kReferenceFixedPropertyCount);
  reference_properties_.clear();
  reference_properties_.reserve(static_cast<std::size_t>(repaired_count));

  for (int i = 0; i < repaired_count; ++i) {
    ReferenceConfig value;
    if (static_cast<std::size_t>(i) < values.size()) {
      value = values[static_cast<std::size_t>(i)];
    }

    ReferencePropertySet properties;
    const QString name = "Reference " + QString::number(i + 1);
    properties.root = new ListItemBoolProperty(
      name, value.enabled, "Enable this reference line.", references_root_property_,
      SLOT(onReferencePropertyChanged()), this);
    properties.duplicate = new rviz_common::properties::BoolProperty(
      "Duplicate", false, "Duplicate this reference.",
      properties.root, SLOT(onDuplicateReferenceChanged()), this);
    properties.duplicate->setShouldBeSaved(false);
    properties.delete_reference = new rviz_common::properties::BoolProperty(
      "Delete", false, "Delete this reference.",
      properties.root, SLOT(onDeleteReferenceChanged()), this);
    properties.delete_reference->setShouldBeSaved(false);
    properties.value = new rviz_common::properties::FloatProperty(
      kReferenceValuePropertyName, value.value, "Y-axis value for this reference line.",
      properties.root,
      SLOT(onReferencePropertyChanged()), this);
    properties.tolerance = new rviz_common::properties::FloatProperty(
      "Tolerance", value.tolerance,
      "Optional symmetric tolerance around the reference value.", properties.root,
      SLOT(onReferencePropertyChanged()), this);
    properties.tolerance->setMin(0.0F);
    properties.label = new rviz_common::properties::StringProperty(
      "Label", QString::fromStdString(value.label), "Reference label.",
      properties.root, SLOT(onReferencePropertyChanged()), this);
    properties.color = new rviz_common::properties::ColorProperty(
      "Color", toQColor(value.color), "Reference line color.",
      properties.root, SLOT(onReferencePropertyChanged()), this);
    properties.alpha = new rviz_common::properties::FloatProperty(
      "Alpha", value.alpha, "Reference line opacity from 0 to 1.",
      properties.root, SLOT(onReferencePropertyChanged()), this);
    properties.alpha->setMin(0.0F);
    properties.alpha->setMax(1.0F);
    properties.line_width = new rviz_common::properties::FloatProperty(
      "Line Width", value.line_width, "Reference line width in pixels.",
      properties.root, SLOT(onReferencePropertyChanged()), this);
    properties.line_width->setMin(static_cast<float>(kMinimumLineWidth));
    properties.line_width->setMax(static_cast<float>(kMaximumLineWidth));
    properties.line_style = new rviz_common::properties::EnumProperty(
      "Line Style", QString::fromStdString(lineStyleName(value.line_style)),
      "Reference line pattern.", properties.root, SLOT(onReferencePropertyChanged()), this);
    addLineStyleOptions(properties.line_style);
    reference_properties_.push_back(properties);
  }
  rebuilding_reference_properties_ = false;
  updateReferencePropertySummaries_();
}

void Plot2DDisplay::replaceReferenceProperties_(const std::vector<ReferenceConfig> & values)
{
  const int count = static_cast<int>(std::clamp<std::size_t>(values.size(), 0U, 12U));
  {
    const QSignalBlocker blocker(reference_count_property_);
    reference_count_property_->setInt(count);
  }
  rebuildReferenceProperties_(count, values);
}

void Plot2DDisplay::appendReferencePreset_()
{
  const double preset_value = reference_preset_value_property_ ?
    reference_preset_value_property_->getFloat() : 0.0;
  const double preset_tolerance = reference_preset_tolerance_property_ ?
    reference_preset_tolerance_property_->getFloat() : 0.0;
  std::vector<ReferenceConfig> additions = referencePresetFromName(
    reference_preset_property_->getStdString(), preset_value, preset_tolerance);
  if (additions.empty()) {
    return;
  }

  std::vector<ReferenceConfig> references = referenceConfigFromProperties_();
  for (const ReferenceConfig & reference : additions) {
    if (references.size() >= 12U) {
      break;
    }
    references.push_back(reference);
  }

  replaceReferenceProperties_(references);
  onReferencePropertyChanged();
}

void Plot2DDisplay::updateModePropertyVisibility_()
{
  const bool xy_mode = plot_mode_property_ &&
    plotModeFromName(plot_mode_property_->getStdString()) == PlotMode::XY;

  if (xy_history_mode_property_) {
    xy_history_mode_property_->setHidden(!xy_mode);
  }
  if (x_axis_root_property_) {
    x_axis_root_property_->setHidden(!xy_mode);
  }

  for (SeriesPropertySet & series : series_properties_) {
    if (series.field) {
      series.field->setHidden(xy_mode);
    }
    if (series.x_field) {
      series.x_field->setHidden(!xy_mode);
    }
    if (series.y_field) {
      series.y_field->setHidden(!xy_mode);
    }
  }
}

void Plot2DDisplay::updateSeriesPropertySummaries_()
{
  const PlotMode plot_mode = plot_mode_property_ ?
    plotModeFromName(plot_mode_property_->getStdString()) : PlotMode::TimeSeries;
  const std::vector<SeriesConfig> series = seriesConfigFromProperties_();
  for (std::size_t i = 0; i < series_properties_.size() && i < series.size(); ++i) {
    auto * root = dynamic_cast<ListItemBoolProperty *>(series_properties_[i].root);
    if (!root) {
      continue;
    }

    QString label = QString::fromStdString(series[i].label);
    if (label.isEmpty()) {
      label = QString::fromStdString(seriesDefaultLabel(series[i], plot_mode));
    }
    if (label.isEmpty()) {
      label = "Series " + QString::number(static_cast<int>(i) + 1);
    }
    root->setDisplayLabel(label);
  }
}

void Plot2DDisplay::updateReferencePropertySummaries_()
{
  const std::vector<ReferenceConfig> references = referenceConfigFromProperties_();
  for (std::size_t i = 0; i < reference_properties_.size() && i < references.size(); ++i) {
    auto * root = dynamic_cast<ListItemBoolProperty *>(reference_properties_[i].root);
    if (!root) {
      continue;
    }

    QString label = QString::fromStdString(references[i].label);
    if (label.isEmpty()) {
      label = "Reference " + QString::number(static_cast<int>(i) + 1);
    }
    root->setDisplayLabel(label);
  }
}

const Plot2DDisplay::SeriesPropertySet * Plot2DDisplay::seriesPropertiesForField_(
  rviz_common::properties::EditableEnumProperty * property) const
{
  for (const SeriesPropertySet & series : series_properties_) {
    if (series.field == property || series.x_field == property || series.y_field == property) {
      return &series;
    }
  }
  return nullptr;
}

void Plot2DDisplay::onSeriesChildListChanged_(
  rviz_common::properties::Property * property)
{
  if (property == series_root_property_ && !rebuilding_series_properties_) {
    scheduleSeriesOrderSync_();
  }
}

void Plot2DDisplay::onReferenceChildListChanged_(
  rviz_common::properties::Property * property)
{
  if (property == references_root_property_ && !rebuilding_reference_properties_) {
    scheduleReferenceOrderSync_();
  }
}

void Plot2DDisplay::scheduleSeriesOrderSync_()
{
  if (series_order_sync_pending_) {
    return;
  }
  series_order_sync_pending_ = true;
  QTimer::singleShot(
    0,
    this,
    [this]() {
      series_order_sync_pending_ = false;
      if (syncSeriesPropertyOrder_()) {
        onConfigPropertyChanged();
      }
    });
}

void Plot2DDisplay::scheduleReferenceOrderSync_()
{
  if (reference_order_sync_pending_) {
    return;
  }
  reference_order_sync_pending_ = true;
  QTimer::singleShot(
    0,
    this,
    [this]() {
      reference_order_sync_pending_ = false;
      if (syncReferencePropertyOrder_()) {
        onReferencePropertyChanged();
      }
    });
}

bool Plot2DDisplay::syncSeriesPropertyOrder_()
{
  if (!syncPropertyOrder(series_root_property_, 1, series_properties_, "Series ")) {
    return false;
  }
  updateSeriesPropertySummaries_();
  return true;
}

bool Plot2DDisplay::syncReferencePropertyOrder_()
{
  if (!syncPropertyOrder(
      references_root_property_, kReferenceFixedPropertyCount, reference_properties_, "Reference "))
  {
    return false;
  }
  updateReferencePropertySummaries_();
  return true;
}

void Plot2DDisplay::resolveAndSubscribe_()
{
  const TopicTypeMap topics = topicNamesAndTypes_();
  const Plot2DConfig config = configFromProperties_();

  Plot2DControllerState state;
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    controller_.configure(config, topics);
    state = controller_.state();
  }

  subscription_manager_->clear();
  const std::vector<PlotSubscriptionTarget> subscription_targets =
    subscriptionTargetsFromControllerState(state);

  if (subscription_targets.empty()) {
    updateStatusFromController_(state);
    return;
  }

  if (!subscription_manager_->hasFactory()) {
    updateStatusFromController_(state);
    return;
  }

  try {
    subscription_manager_->subscribe(
      subscription_targets,
      config.qos,
      [this](
        const std::string & topic,
        std::shared_ptr<rclcpp::SerializedMessage> message)
      {
        onSerializedMessage_(topic, std::move(message));
      });
  } catch (const std::exception & exception) {
    setStatus(
      rviz_common::properties::StatusProperty::Error,
      "Subscriptions",
      QString::fromStdString(exception.what()));
    return;
  }

  updateStatusFromController_(state);
}

void Plot2DDisplay::onSerializedMessage_(
  const std::string & topic,
  std::shared_ptr<rclcpp::SerializedMessage> message)
{
  if (!message) {
    return;
  }

  Plot2DControllerState state;
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    controller_.appendSerializedMessage(topic, *message, receiveNowSeconds_());
    state = controller_.state();
  }
  updateStatusFromController_(state);
}

void Plot2DDisplay::updateStatusFromController_()
{
  Plot2DControllerState state;
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    state = controller_.state();
  }
  updateStatusFromController_(state);
}

void Plot2DDisplay::updateStatusFromController_(const Plot2DControllerState & state)
{
  setStatus(
    statusLevel(state.status),
    "Series 1",
    QString::fromStdString(statusText(state)));
}

Plot2DDisplay::RenderSnapshot Plot2DDisplay::renderSnapshot_() const
{
  const Plot2DConfig config = configFromProperties_();
  Plot2DControllerState state;
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    state = controller_.state();
  }
  return RenderSnapshot{config, state};
}

PlotRenderSettings Plot2DDisplay::renderSettingsFromProperties_() const
{
  return renderSettingsFromConfig_(configFromProperties_());
}

PlotRenderSettings Plot2DDisplay::renderSettingsFromConfig_(const Plot2DConfig & config) const
{
  PlotRenderSettings settings;
  settings.width = config.layout.width;
  settings.height = config.layout.height;
  settings.window_seconds = config.time.window_seconds;
  settings.now = plotNowSeconds_(config.time.source);
  settings.x_axis_mode = config.x_axis.mode;
  settings.x_scale_mode = config.x_axis.scale_mode;
  settings.xy_axis_scale_mode = config.x_axis.axis_scale_mode;
  settings.fixed_x_min = config.x_axis.fixed_min;
  settings.fixed_x_max = config.x_axis.fixed_max;
  settings.x_padding_fraction = config.x_axis.padding_fraction;
  settings.y_scale_mode = config.y_axis.scale_mode;
  settings.fixed_y_min = config.y_axis.fixed_min;
  settings.fixed_y_max = config.y_axis.fixed_max;
  settings.y_padding_fraction = config.y_axis.padding_fraction;
  settings.background_color = background_color_property_->getColor();
  const float background_alpha = background_alpha_property_ ?
    background_alpha_property_->getFloat() : 190.0F / 255.0F;
  settings.background_color.setAlphaF(std::clamp(background_alpha, 0.0F, 1.0F));
  settings.axis_color = axis_color_property_->getColor();
  settings.axis_color.setAlpha(230);
  settings.grid_color = grid_color_property_->getColor();
  settings.grid_color.setAlpha(80);
  settings.text_color = text_color_property_->getColor();
  settings.text_color.setAlpha(235);
  settings.font_size = font_size_property_ ? font_size_property_->getInt() : 8;
  settings.show_legend = show_legend_property_ ? show_legend_property_->getBool() : true;
  settings.show_latest_values = show_latest_values_property_ ?
    show_latest_values_property_->getBool() : true;
  settings.legend_position = legend_position_property_ ?
    legendPositionFromName(legend_position_property_->getStdString()) : LegendPosition::TopLeft;
  settings.legend_x_offset = legend_x_offset_property_ ? legend_x_offset_property_->getInt() : 4;
  settings.legend_y_offset = legend_y_offset_property_ ? legend_y_offset_property_->getInt() : 4;
  settings.show_major_grid = show_major_grid_property_ ?
    show_major_grid_property_->getBool() : true;
  settings.show_minor_grid = show_minor_grid_property_ ?
    show_minor_grid_property_->getBool() : true;
  settings.x_major_tick_count = x_major_tick_count_property_ ?
    x_major_tick_count_property_->getInt() : 6;
  settings.y_major_tick_count = y_major_tick_count_property_ ?
    y_major_tick_count_property_->getInt() : 5;
  settings.minor_grid_divisions = minor_grid_divisions_property_ ?
    minor_grid_divisions_property_->getInt() : 1;
  return settings;
}

std::vector<RenderableSeries> Plot2DDisplay::renderableSeries_() const
{
  return renderableSeriesFromSnapshot_(renderSnapshot_());
}

std::vector<RenderableSeries> Plot2DDisplay::renderableSeriesFromSnapshot_(
  const RenderSnapshot & snapshot) const
{
  std::vector<RenderableSeries> output;
  output.reserve(snapshot.controller_state.series.size());
  for (std::size_t i = 0; i < snapshot.controller_state.series.size(); ++i) {
    const PlotSeriesControllerState & source = snapshot.controller_state.series[i];
    RenderableSeries series;
    series.enabled = i < snapshot.config.series.size() && snapshot.config.series[i].enabled;
    if (i < snapshot.config.series.size()) {
      series.label = snapshot.config.series[i].label.empty() ?
        seriesDefaultLabel(snapshot.config.series[i], snapshot.config.plot_mode) :
        snapshot.config.series[i].label;
      if (series.label.empty()) {
        series.label = "Series";
      }
      series.unit = snapshot.config.series[i].unit;
      series.color = toQColor(snapshot.config.series[i].color);
      series.color.setAlphaF(snapshot.config.series[i].line_alpha);
      series.line_width = snapshot.config.series[i].line_width;
      series.line_style = snapshot.config.series[i].line_style;
      series.plot_style = snapshot.config.series[i].plot_style;
    } else {
      series.label = source.label.empty() ? "Series" : source.label;
    }
    series.samples = source.samples.samples();
    output.push_back(std::move(series));
  }
  return output;
}

std::vector<RenderableReference> Plot2DDisplay::renderableReferences_() const
{
  return renderableReferencesFromConfig_(configFromProperties_());
}

std::vector<RenderableReference> Plot2DDisplay::renderableReferencesFromConfig_(
  const Plot2DConfig & config) const
{
  std::vector<RenderableReference> output;
  output.reserve(config.references.size());
  for (const ReferenceConfig & source : config.references) {
    RenderableReference reference;
    reference.enabled = source.enabled;
    reference.value = source.value;
    reference.tolerance = source.tolerance;
    reference.label = source.label;
    reference.color = toQColor(source.color);
    reference.color.setAlphaF(source.alpha);
    reference.line_width = source.line_width;
    reference.line_style = source.line_style;
    output.push_back(std::move(reference));
  }
  return output;
}

void Plot2DDisplay::initializeOverlayBackend_()
{
  if (!overlay_backend_factory_) {
    return;
  }

  std::ostringstream name;
  name << "rviz_2d_plot_overlay_"
       << reinterpret_cast<std::uintptr_t>(this);
  overlay_backend_ = overlay_backend_factory_(name.str());
  if (!overlay_backend_) {
    return;
  }

  const OverlayBackendResult initialize_result =
    overlay_backend_->initialize(scene_manager_);
  if (!initialize_result.ok()) {
    setStatus(
      rviz_common::properties::StatusProperty::Error,
      "Overlay",
      QString::fromStdString(initialize_result.message));
    overlay_backend_.reset();
    return;
  }

  updateOverlayGeometry_();
  overlay_backend_->setVisible(false);
}

void Plot2DDisplay::updateOverlayGeometry_()
{
  updateOverlayGeometry_(configFromProperties_());
}

void Plot2DDisplay::updateOverlayGeometry_(const Plot2DConfig & config)
{
  if (!overlay_backend_) {
    return;
  }

  OverlayGeometry geometry;
  geometry.width = config.layout.width;
  geometry.height = config.layout.height;
  geometry.x_offset = config.layout.x_offset;
  geometry.y_offset = config.layout.y_offset;
  geometry.horizontal_alignment =
    toOverlayHorizontalAlignment(config.layout.horizontal_alignment);
  geometry.vertical_alignment =
    toOverlayVerticalAlignment(config.layout.vertical_alignment);

  const OverlayBackendResult geometry_result =
    overlay_backend_->setGeometry(geometry);
  if (!geometry_result.ok()) {
    setStatus(
      rviz_common::properties::StatusProperty::Error,
      "Overlay",
      QString::fromStdString(geometry_result.message));
  }
}

void Plot2DDisplay::renderOverlay_(const bool request_rviz_render)
{
  if (!overlay_backend_) {
    return;
  }

  const RenderSnapshot snapshot = renderSnapshot_();
  updateOverlayGeometry_(snapshot.config);
  if (isEnabled()) {
    overlay_backend_->setVisible(true);
  }
  if (!overlay_backend_->isReady()) {
    return;
  }

  const PlotRenderSettings settings = renderSettingsFromConfig_(snapshot.config);
  const QImage rendered = renderer_.render(
    settings,
    renderableSeriesFromSnapshot_(snapshot),
    renderableReferencesFromConfig_(snapshot.config));

  const OverlayBackendResult image_result = overlay_backend_->updateImage(rendered);
  if (!image_result.ok()) {
    setStatus(
      rviz_common::properties::StatusProperty::Error,
      "Overlay",
      QString::fromStdString(image_result.message));
    return;
  }

  if (request_rviz_render && context_) {
    context_->queueRender();
  }
}

void Plot2DDisplay::unsubscribe_()
{
  subscription_manager_->clear();
}

bool Plot2DDisplay::shouldRetrySubscriptions_() const
{
  std::lock_guard<std::mutex> lock(controller_mutex_);
  const PlotControllerStatus status = controller_.state().status;
  return status == PlotControllerStatus::WaitingForTopic ||
         status == PlotControllerStatus::AmbiguousTopicType;
}

double Plot2DDisplay::receiveNowSeconds_() const
{
  if (node_) {
    return node_->get_clock()->now().seconds();
  }
  return rclcpp::Clock().now().seconds();
}

double Plot2DDisplay::plotNowSeconds_(const TimeSource source) const
{
  if (source != TimeSource::HeaderStamp) {
    return receiveNowSeconds_();
  }

  double newest_sample_time = -std::numeric_limits<double>::infinity();
  bool has_sample = false;
  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    for (const PlotSeriesControllerState & series : controller_.state().series) {
      const std::optional<PlotSample> latest = series.samples.latest();
      if (!latest.has_value()) {
        continue;
      }
      newest_sample_time = std::max(newest_sample_time, latest->time);
      has_sample = true;
    }
  }

  if (has_sample) {
    return newest_sample_time;
  }
  return receiveNowSeconds_();
}

TopicTypeMap Plot2DDisplay::topicNamesAndTypes_() const
{
  if (!ros_graph_ops_.get_topic_names_and_types) {
    return {};
  }

  try {
    return ros_graph_ops_.get_topic_names_and_types();
  } catch (const std::exception &) {
    return {};
  } catch (...) {
    return {};
  }
}

std::vector<std::string> Plot2DDisplay::topicOptions_() const
{
  const TopicTypeMap topics = topicNamesAndTypes_();

  std::vector<std::string> options;
  options.reserve(topics.size());
  for (const auto & [topic, types] : topics) {
    (void)types;
    options.push_back(topic);
  }
  std::sort(options.begin(), options.end());
  return options;
}

std::vector<std::string> Plot2DDisplay::fieldOptionsForTopic_(
  const std::string & topic) const
{
  const TopicTypeMap topics = topicNamesAndTypes_();
  const auto topic_it = topics.find(topic);
  if (topic_it == topics.end() || topic_it->second.size() != 1U) {
    return {};
  }

  FieldPathOptions fields = numericScalarFieldPathsForType(topic_it->second.front());
  std::sort(fields.paths.begin(), fields.paths.end());
  return fields.paths;
}

}  // namespace rviz_2d_plot_plugin

PLUGINLIB_EXPORT_CLASS(rviz_2d_plot_plugin::Plot2DDisplay, rviz_common::Display)
