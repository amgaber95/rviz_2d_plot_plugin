// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_display.hpp"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QSignalBlocker>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <sstream>
#include <utility>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_2d_overlay_plugins/overlay_utils.hpp>
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
#include <rviz_rendering/render_system.hpp>

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

SeriesColor defaultSeriesColor(const std::size_t index)
{
  static const std::vector<SeriesColor> palette{
    SeriesColor{80, 170, 255},
    SeriesColor{80, 220, 130},
    SeriesColor{255, 170, 60},
    SeriesColor{210, 130, 255},
    SeriesColor{255, 90, 90},
    SeriesColor{120, 220, 255},
    SeriesColor{230, 230, 90},
    SeriesColor{160, 160, 255},
  };
  return palette[index % palette.size()];
}

QColor toQColor(const SeriesColor & color)
{
  return QColor(color.red, color.green, color.blue);
}

SeriesColor toSeriesColor(const QColor & color)
{
  return SeriesColor{color.red(), color.green(), color.blue()};
}

std::string lineStyleName(const LineStyle style)
{
  switch (style) {
    case LineStyle::Solid:
      return "Solid";
    case LineStyle::Dash:
      return "Dash";
    case LineStyle::Dot:
      return "Dot";
    case LineStyle::DashDot:
      return "Dash Dot";
  }
  return "Solid";
}

LineStyle lineStyleFromName(const std::string & name)
{
  if (name == "Dash") {
    return LineStyle::Dash;
  }
  if (name == "Dot") {
    return LineStyle::Dot;
  }
  if (name == "Dash Dot") {
    return LineStyle::DashDot;
  }
  return LineStyle::Solid;
}

void addLineStyleOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(lineStyleName(LineStyle::Solid));
  property->addOptionStd(lineStyleName(LineStyle::Dash));
  property->addOptionStd(lineStyleName(LineStyle::Dot));
  property->addOptionStd(lineStyleName(LineStyle::DashDot));
}

std::string plotStyleName(const PlotStyle style)
{
  switch (style) {
    case PlotStyle::Line:
      return "Line";
    case PlotStyle::Step:
      return "Step";
    case PlotStyle::Points:
      return "Points";
  }
  return "Line";
}

PlotStyle plotStyleFromName(const std::string & name)
{
  if (name == "Step") {
    return PlotStyle::Step;
  }
  if (name == "Points") {
    return PlotStyle::Points;
  }
  return PlotStyle::Line;
}

void addPlotStyleOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(plotStyleName(PlotStyle::Line));
  property->addOptionStd(plotStyleName(PlotStyle::Step));
  property->addOptionStd(plotStyleName(PlotStyle::Points));
}

constexpr const char * kNoReferencePreset = "None";

void addReferencePresetOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(kNoReferencePreset);
  property->addOptionStd("Zero Line");
  property->addOptionStd("Upper Limit");
  property->addOptionStd("Lower Limit");
  property->addOptionStd("Symmetric Limits");
}

std::vector<ReferenceConfig> referencePresetFromName(
  const std::string & name,
  const double preset_value)
{
  const double value = std::isfinite(preset_value) ? preset_value : 0.0;

  if (name == "Zero Line") {
    ReferenceConfig reference;
    reference.value = 0.0;
    reference.label = "Zero";
    reference.color = SeriesColor{180, 180, 180};
    reference.alpha = 0.65;
    reference.line_width = 1.0;
    reference.line_style = LineStyle::Dot;
    return {reference};
  }

  if (name == "Upper Limit") {
    ReferenceConfig reference;
    reference.value = value;
    reference.label = "Upper Limit";
    reference.color = SeriesColor{255, 180, 60};
    reference.alpha = 0.9;
    reference.line_width = 1.2;
    reference.line_style = LineStyle::Dash;
    return {reference};
  }

  if (name == "Lower Limit") {
    ReferenceConfig reference;
    reference.value = value;
    reference.label = "Lower Limit";
    reference.color = SeriesColor{80, 170, 255};
    reference.alpha = 0.9;
    reference.line_width = 1.2;
    reference.line_style = LineStyle::Dash;
    return {reference};
  }

  if (name == "Symmetric Limits") {
    const double magnitude = std::abs(value);
    ReferenceConfig upper;
    upper.value = magnitude;
    upper.label = "Upper Limit";
    upper.color = SeriesColor{255, 180, 60};
    upper.alpha = 0.9;
    upper.line_width = 1.2;
    upper.line_style = LineStyle::Dash;

    ReferenceConfig lower;
    lower.value = -magnitude;
    lower.label = "Lower Limit";
    lower.color = SeriesColor{80, 170, 255};
    lower.alpha = 0.9;
    lower.line_width = 1.2;
    lower.line_style = LineStyle::Dash;
    return {upper, lower};
  }

  return {};
}

}  // namespace

Plot2DDisplay::Plot2DDisplay()
{
  overlay_backend_ops_.prepare_overlays =
    [](Ogre::SceneManager * scene_manager)
    {
      rviz_rendering::RenderSystem::get()->prepareOverlays(scene_manager);
    };

  pause_plot_property_ = new rviz_common::properties::BoolProperty(
    "Pause Plot", false, "Pause incoming sample collection and hold the plot.",
    this, SLOT(onConfigPropertyChanged()), this);
  clear_history_property_ = new rviz_common::properties::BoolProperty(
    "Clear History", false, "Clear stored samples for this plot.",
    this, SLOT(onClearHistoryChanged()), this);

  series_root_property_ = new rviz_common::properties::Property(
    "Series", QVariant(), "Topic field series to draw.", this);
  series_count_property_ = new rviz_common::properties::IntProperty(
    "Series Count", 1, "Number of plotted topic fields.",
    series_root_property_, SLOT(onSeriesCountChanged()), this, 1, 12);
  rebuildSeriesProperties_(1, {SeriesConfig{}});

  time_root_property_ = new rviz_common::properties::Property(
    "Time", QVariant(), "Time-series history and redraw settings.", this);
  window_seconds_property_ = new rviz_common::properties::FloatProperty(
    "Window Seconds", 30.0F, "Visible rolling time window in seconds.",
    time_root_property_, SLOT(onConfigPropertyChanged()), this);
  window_seconds_property_->setMin(1.0F);
  refresh_rate_property_ = new rviz_common::properties::FloatProperty(
    "Refresh Rate", 20.0F, "Overlay redraw rate in Hz.", time_root_property_,
    SLOT(onConfigPropertyChanged()), this);
  refresh_rate_property_->setMin(1.0F);

  y_axis_root_property_ = new rviz_common::properties::Property(
    "Y Axis", QVariant(), "Vertical value axis scaling.", this);
  auto_scale_property_ = new rviz_common::properties::BoolProperty(
    "Auto Scale", true, "Automatically fit the y-axis to visible samples.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  y_min_property_ = new rviz_common::properties::FloatProperty(
    "Y Min", -1.0F, "Fixed y-axis minimum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  y_max_property_ = new rviz_common::properties::FloatProperty(
    "Y Max", 1.0F, "Fixed y-axis maximum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);

  references_root_property_ = new rviz_common::properties::Property(
    "References", QVariant(), "Horizontal reference lines.", this);
  reference_preset_property_ = new rviz_common::properties::EnumProperty(
    "Add Preset", kNoReferencePreset, "Append a common reference line preset.",
    references_root_property_, SLOT(onReferencePresetChanged()), this);
  addReferencePresetOptions(reference_preset_property_);
  reference_preset_value_property_ = new rviz_common::properties::FloatProperty(
    "Preset Value", 1.0F,
    "Y-axis value used by single-line presets. Symmetric limits use +/- this value.",
    references_root_property_);
  reference_count_property_ = new rviz_common::properties::IntProperty(
    "Reference Count", 0, "Number of horizontal reference lines.",
    references_root_property_, SLOT(onReferenceCountChanged()), this, 0, 12);
  rebuildReferenceProperties_(0, {});

  layout_root_property_ = new rviz_common::properties::Property(
    "Layout", QVariant(), "Overlay size and screen position.", this);
  width_property_ = new rviz_common::properties::IntProperty(
    "Width", 360, "Overlay width in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this, 120);
  height_property_ = new rviz_common::properties::IntProperty(
    "Height", 220, "Overlay height in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this, 80);
  x_offset_property_ = new rviz_common::properties::IntProperty(
    "X Offset", 10, "Horizontal screen offset in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this);
  y_offset_property_ = new rviz_common::properties::IntProperty(
    "Y Offset", 10, "Vertical screen offset in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this);

  style_root_property_ = new rviz_common::properties::Property(
    "Style", QVariant(), "Plot colors.", this);
  background_color_property_ = new rviz_common::properties::ColorProperty(
    "Background Color", QColor(0, 0, 0), "Plot background color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  axis_color_property_ = new rviz_common::properties::ColorProperty(
    "Axis Color", QColor(230, 230, 230), "Axis and border color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  grid_color_property_ = new rviz_common::properties::ColorProperty(
    "Grid Color", QColor(130, 130, 130), "Grid line color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  text_color_property_ = new rviz_common::properties::ColorProperty(
    "Text Color", QColor(245, 245, 245), "Axis and legend text color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
}

Plot2DDisplay::~Plot2DDisplay() = default;

void Plot2DDisplay::onInitialize()
{
  rviz_common::Display::onInitialize();
  auto rviz_node = context_->getRosNodeAbstraction().lock();
  if (rviz_node) {
    node_ = rviz_node->get_raw_node();
  }

  if (node_) {
    ros_graph_ops_.get_topic_names_and_types = [this]() {
        const auto context = node_->get_node_base_interface()->get_context();
        if (!context || !context->is_valid()) {
          return TopicTypeMap{};
        }
        return node_->get_topic_names_and_types();
      };
    subscription_factory_.create_generic_subscription =
      [this](
      const std::string & topic,
      const std::string & type,
      rclcpp::QoS qos,
      SerializedMessageCallback callback)
      {
        return node_->create_generic_subscription(
          topic, type, qos, std::move(callback));
      };
  }

  std::ostringstream name;
  name << "rviz_2d_plot_overlay_"
       << reinterpret_cast<std::uintptr_t>(this);
  prepareOverlayRendering_();
  overlay_ =
    std::make_shared<rviz_2d_overlay_plugins::OverlayObject>(name.str());
  overlay_->updateTextureSize(360, 220);
  overlay_->setDimensions(360, 220);
  overlay_->hide();
  resolveAndSubscribe_();
}

void Plot2DDisplay::onEnable()
{
  resolveAndSubscribe_();
  if (overlay_) {
    overlay_->show();
  }
  renderOverlay_();
}

void Plot2DDisplay::onDisable()
{
  unsubscribe_();
  if (overlay_) {
    overlay_->hide();
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
    renderOverlay_();
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
  resolveAndSubscribe_();
  renderOverlay_();
}

void Plot2DDisplay::onSeriesCountChanged()
{
  const std::vector<SeriesConfig> current = seriesConfigFromProperties_();
  rebuildSeriesProperties_(series_count_property_->getInt(), current);
  onConfigPropertyChanged();
}

void Plot2DDisplay::onReferencePresetChanged()
{
  if (!reference_preset_property_) {
    return;
  }

  const std::string preset = reference_preset_property_->getStdString();
  if (preset == kNoReferencePreset) {
    return;
  }

  appendReferencePreset_();
  const QSignalBlocker blocker(reference_preset_property_);
  reference_preset_property_->setValue(kNoReferencePreset);
}

void Plot2DDisplay::onReferenceCountChanged()
{
  const std::vector<ReferenceConfig> current = referenceConfigFromProperties_();
  rebuildReferenceProperties_(reference_count_property_->getInt(), current);
  onConfigPropertyChanged();
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
    config.enabled = properties.enabled && properties.enabled->getBool();
    config.topic = properties.topic ? properties.topic->getStdString() : "";
    config.field = properties.field ? properties.field->getStdString() : "";
    config.label = properties.label ? properties.label->getStdString() : "Series";
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
    config.enabled = properties.enabled && properties.enabled->getBool();
    config.value = properties.value ? properties.value->getFloat() : 0.0;
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

  config.time.window_seconds = window_seconds_property_->getFloat();
  config.time.refresh_rate_hz = refresh_rate_property_->getFloat();
  config.time.paused = pause_plot_property_->getBool();

  config.y_axis.scale_mode = auto_scale_property_->getBool() ?
    AxisScaleMode::Auto : AxisScaleMode::Fixed;
  config.y_axis.fixed_min = y_min_property_->getFloat();
  config.y_axis.fixed_max = y_max_property_->getFloat();

  config.layout.width = width_property_->getInt();
  config.layout.height = height_property_->getInt();
  config.layout.x_offset = x_offset_property_->getInt();
  config.layout.y_offset = y_offset_property_->getInt();
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

  series_root_property_->removeChildren(1);
  series_properties_.clear();
  series_properties_.reserve(static_cast<std::size_t>(repaired_count));

  for (int i = 0; i < repaired_count; ++i) {
    SeriesConfig value;
    if (static_cast<std::size_t>(i) < values.size()) {
      value = values[static_cast<std::size_t>(i)];
    } else {
      value.label = "Series " + std::to_string(i + 1);
      value.color = defaultSeriesColor(static_cast<std::size_t>(i));
    }

    SeriesPropertySet properties;
    const QString name = "Series " + QString::number(i + 1);
    properties.root = new rviz_common::properties::Property(
      name, QVariant(), "Plotted topic field.", series_root_property_);
    properties.enabled = new rviz_common::properties::BoolProperty(
      "Enabled", value.enabled, "Enable this series.", properties.root,
      SLOT(onConfigPropertyChanged()), this);
    properties.topic = new rviz_common::properties::EditableEnumProperty(
      "Topic", QString::fromStdString(value.topic), "ROS 2 topic to subscribe to.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.topic,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onTopicOptionsRequested);
    properties.field = new rviz_common::properties::EditableEnumProperty(
      "Field", QString::fromStdString(value.field),
      "Numeric or boolean field path inside the selected message.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    QObject::connect(
      properties.field,
      &rviz_common::properties::EditableEnumProperty::requestOptions,
      this,
      &Plot2DDisplay::onFieldOptionsRequested);
    properties.label = new rviz_common::properties::StringProperty(
      "Label", QString::fromStdString(value.label), "Legend label for this series.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.color = new rviz_common::properties::ColorProperty(
      "Color", toQColor(value.color), "Series line color.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.line_width = new rviz_common::properties::FloatProperty(
      "Line Width", value.line_width, "Series line width in pixels.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.line_width->setMin(1.0F);
    properties.line_alpha = new rviz_common::properties::FloatProperty(
      "Line Alpha", value.line_alpha, "Series line opacity from 0 to 1.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.line_alpha->setMin(0.0F);
    properties.line_alpha->setMax(1.0F);
    properties.line_style = new rviz_common::properties::EnumProperty(
      "Line Style", QString::fromStdString(lineStyleName(value.line_style)),
      "Series line pattern.", properties.root, SLOT(onConfigPropertyChanged()), this);
    addLineStyleOptions(properties.line_style);
    properties.plot_style = new rviz_common::properties::EnumProperty(
      "Plot Style", QString::fromStdString(plotStyleName(value.plot_style)),
      "Series rendering mode.", properties.root, SLOT(onConfigPropertyChanged()), this);
    addPlotStyleOptions(properties.plot_style);
    properties.value_scale = new rviz_common::properties::FloatProperty(
      "Value Scale", value.value_scale, "Scale applied to extracted values before plotting.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.value_offset = new rviz_common::properties::FloatProperty(
      "Value Offset", value.value_offset, "Offset added after scaling extracted values.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    series_properties_.push_back(properties);
  }
}

void Plot2DDisplay::rebuildReferenceProperties_(
  const int count,
  const std::vector<ReferenceConfig> & values)
{
  const int repaired_count = std::clamp(count, 0, 12);
  if (reference_count_property_->getInt() != repaired_count) {
    reference_count_property_->setInt(repaired_count);
  }

  references_root_property_->removeChildren(3);
  reference_properties_.clear();
  reference_properties_.reserve(static_cast<std::size_t>(repaired_count));

  for (int i = 0; i < repaired_count; ++i) {
    ReferenceConfig value;
    if (static_cast<std::size_t>(i) < values.size()) {
      value = values[static_cast<std::size_t>(i)];
    }

    ReferencePropertySet properties;
    const QString name = "Reference " + QString::number(i + 1);
    properties.root = new rviz_common::properties::Property(
      name, QVariant(), "Horizontal reference line.", references_root_property_);
    properties.enabled = new rviz_common::properties::BoolProperty(
      "Enabled", value.enabled, "Enable this reference line.", properties.root,
      SLOT(onConfigPropertyChanged()), this);
    properties.value = new rviz_common::properties::FloatProperty(
      "Value", value.value, "Y-axis value for this reference line.", properties.root,
      SLOT(onConfigPropertyChanged()), this);
    properties.label = new rviz_common::properties::StringProperty(
      "Label", QString::fromStdString(value.label), "Reference label.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.color = new rviz_common::properties::ColorProperty(
      "Color", toQColor(value.color), "Reference line color.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.alpha = new rviz_common::properties::FloatProperty(
      "Alpha", value.alpha, "Reference line opacity from 0 to 1.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.alpha->setMin(0.0F);
    properties.alpha->setMax(1.0F);
    properties.line_width = new rviz_common::properties::FloatProperty(
      "Line Width", value.line_width, "Reference line width in pixels.",
      properties.root, SLOT(onConfigPropertyChanged()), this);
    properties.line_width->setMin(1.0F);
    properties.line_style = new rviz_common::properties::EnumProperty(
      "Line Style", QString::fromStdString(lineStyleName(value.line_style)),
      "Reference line pattern.", properties.root, SLOT(onConfigPropertyChanged()), this);
    addLineStyleOptions(properties.line_style);
    reference_properties_.push_back(properties);
  }
}

void Plot2DDisplay::appendReferencePreset_()
{
  const double preset_value = reference_preset_value_property_ ?
    reference_preset_value_property_->getFloat() : 0.0;
  std::vector<ReferenceConfig> additions = referencePresetFromName(
    reference_preset_property_->getStdString(), preset_value);
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

  {
    const QSignalBlocker blocker(reference_count_property_);
    reference_count_property_->setInt(static_cast<int>(references.size()));
  }
  rebuildReferenceProperties_(static_cast<int>(references.size()), references);
  onConfigPropertyChanged();
}

const Plot2DDisplay::SeriesPropertySet * Plot2DDisplay::seriesPropertiesForField_(
  rviz_common::properties::EditableEnumProperty * property) const
{
  for (const SeriesPropertySet & series : series_properties_) {
    if (series.field == property) {
      return &series;
    }
  }
  return nullptr;
}

void Plot2DDisplay::resolveAndSubscribe_()
{
  const TopicTypeMap topics = topicNamesAndTypes_();
  const Plot2DConfig config = configFromProperties_();

  std::lock_guard<std::mutex> lock(controller_mutex_);
  controller_.configure(config, topics);
  subscriptions_.clear();

  const Plot2DControllerState & state = controller_.state();
  std::vector<std::pair<std::string, std::string>> subscription_topics;
  for (const PlotSeriesControllerState & series : state.series) {
    if (series.status == PlotControllerStatus::Ok) {
      const std::pair<std::string, std::string> topic_type{series.topic, series.type};
      const auto duplicate = std::find(
        subscription_topics.begin(), subscription_topics.end(), topic_type);
      if (duplicate == subscription_topics.end()) {
        subscription_topics.push_back(topic_type);
      }
    }
  }

  if (subscription_topics.empty()) {
    updateStatusFromController_();
    return;
  }

  if (!subscription_factory_.create_generic_subscription) {
    updateStatusFromController_();
    return;
  }

  try {
    for (const auto & [topic, type] : subscription_topics) {
      subscriptions_.push_back(
        subscription_factory_.create_generic_subscription(
          topic,
          type,
          qos_profile_,
          [this, topic](std::shared_ptr<rclcpp::SerializedMessage> message)
          {
            onSerializedMessage_(topic, std::move(message));
          }));
    }
  } catch (const std::exception & exception) {
    setStatus(
      rviz_common::properties::StatusProperty::Error,
      "Subscriptions",
      QString::fromStdString(exception.what()));
    return;
  }

  updateStatusFromController_();
}

void Plot2DDisplay::onSerializedMessage_(
  const std::string & topic,
  std::shared_ptr<rclcpp::SerializedMessage> message)
{
  if (!message) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(controller_mutex_);
    controller_.appendSerializedMessage(topic, *message, receiveNowSeconds_());
  }
  updateStatusFromController_();
  renderOverlay_();
}

void Plot2DDisplay::updateStatusFromController_()
{
  const Plot2DControllerState & state = controller_.state();
  setStatus(
    statusLevel(state.status),
    "Series 1",
    QString::fromStdString(statusText(state)));
}

PlotRenderSettings Plot2DDisplay::renderSettingsFromProperties_() const
{
  const Plot2DConfig config = configFromProperties_();
  PlotRenderSettings settings;
  settings.width = config.layout.width;
  settings.height = config.layout.height;
  settings.window_seconds = config.time.window_seconds;
  settings.now = receiveNowSeconds_();
  settings.y_scale_mode = config.y_axis.scale_mode;
  settings.fixed_y_min = config.y_axis.fixed_min;
  settings.fixed_y_max = config.y_axis.fixed_max;
  settings.y_padding_fraction = config.y_axis.padding_fraction;
  settings.background_color = background_color_property_->getColor();
  settings.background_color.setAlpha(190);
  settings.axis_color = axis_color_property_->getColor();
  settings.axis_color.setAlpha(230);
  settings.grid_color = grid_color_property_->getColor();
  settings.grid_color.setAlpha(80);
  settings.text_color = text_color_property_->getColor();
  settings.text_color.setAlpha(235);
  return settings;
}

std::vector<RenderableSeries> Plot2DDisplay::renderableSeries_() const
{
  const Plot2DConfig config = configFromProperties_();
  std::lock_guard<std::mutex> lock(controller_mutex_);
  const Plot2DControllerState & state = controller_.state();

  std::vector<RenderableSeries> output;
  output.reserve(state.series.size());
  for (std::size_t i = 0; i < state.series.size(); ++i) {
    const PlotSeriesControllerState & source = state.series[i];
    RenderableSeries series;
    series.label = source.label.empty() ? "Series" : source.label;
    series.enabled = source.status == PlotControllerStatus::Ok &&
      i < config.series.size() && config.series[i].enabled;
    if (i < config.series.size()) {
      series.color = toQColor(config.series[i].color);
      series.color.setAlphaF(config.series[i].line_alpha);
      series.line_width = config.series[i].line_width;
      series.line_style = config.series[i].line_style;
      series.plot_style = config.series[i].plot_style;
    }
    series.samples = source.samples.samples();
    output.push_back(std::move(series));
  }
  return output;
}

std::vector<RenderableReference> Plot2DDisplay::renderableReferences_() const
{
  const Plot2DConfig config = configFromProperties_();

  std::vector<RenderableReference> output;
  output.reserve(config.references.size());
  for (const ReferenceConfig & source : config.references) {
    RenderableReference reference;
    reference.enabled = source.enabled;
    reference.value = source.value;
    reference.label = source.label;
    reference.color = toQColor(source.color);
    reference.color.setAlphaF(source.alpha);
    reference.line_width = source.line_width;
    reference.line_style = source.line_style;
    output.push_back(std::move(reference));
  }
  return output;
}

void Plot2DDisplay::updateOverlayGeometry_()
{
  if (!overlay_) {
    return;
  }

  const Plot2DConfig config = configFromProperties_();
  overlay_->updateTextureSize(config.layout.width, config.layout.height);
  overlay_->setDimensions(config.layout.width, config.layout.height);
  overlay_->setPosition(
    config.layout.x_offset,
    config.layout.y_offset,
    rviz_2d_overlay_plugins::HorizontalAlignment::RIGHT,
    rviz_2d_overlay_plugins::VerticalAlignment::TOP);
}

void Plot2DDisplay::renderOverlay_()
{
  if (!overlay_) {
    return;
  }

  updateOverlayGeometry_();
  if (isEnabled()) {
    overlay_->show();
  }
  if (!overlay_->isTextureReady()) {
    return;
  }

  const PlotRenderSettings settings = renderSettingsFromProperties_();
  const QImage rendered = renderer_.render(
    settings,
    renderableSeries_(),
    renderableReferences_());
  QColor clear_color(0, 0, 0, 0);
  auto buffer = overlay_->getBuffer();
  QImage target = buffer.getQImage(
    static_cast<unsigned int>(settings.width),
    static_cast<unsigned int>(settings.height),
    clear_color);
  {
    QPainter painter(&target);
    painter.drawImage(0, 0, rendered);
  }

  if (context_) {
    context_->queueRender();
  }
}

void Plot2DDisplay::unsubscribe_()
{
  subscriptions_.clear();
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

void Plot2DDisplay::prepareOverlayRendering_()
{
  if (overlay_backend_ops_.prepare_overlays) {
    overlay_backend_ops_.prepare_overlays(scene_manager_);
  }
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
