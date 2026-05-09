// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_display.hpp"

#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QSignalBlocker>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <sstream>
#include <utility>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_2d_overlay_plugins/overlay_utils.hpp>
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
#include <rviz_rendering/render_system.hpp>

namespace rviz_2d_plot_plugin
{
namespace
{

class ContainsFilterEditableEnumProperty
  : public rviz_common::properties::EditableEnumProperty
{
public:
  using rviz_common::properties::EditableEnumProperty::EditableEnumProperty;

  QWidget * createEditor(
    QWidget * parent,
    const QStyleOptionViewItem & option) override
  {
    QWidget * editor =
      rviz_common::properties::EditableEnumProperty::createEditor(parent, option);
    auto * combo_box = qobject_cast<QComboBox *>(editor);
    if (combo_box && combo_box->completer()) {
      combo_box->completer()->setCompletionMode(QCompleter::PopupCompletion);
      combo_box->completer()->setCaseSensitivity(Qt::CaseInsensitive);
      combo_box->completer()->setFilterMode(Qt::MatchContains);
    }
    return editor;
  }
};

class ChildOnlyGroupProperty : public rviz_common::properties::Property
{
public:
  using rviz_common::properties::Property::Property;

  void load(const rviz_common::Config & config) override
  {
    if (config.getType() != rviz_common::Config::Map) {
      rviz_common::properties::Property::load(config);
      return;
    }

    const int child_count = numChildren();
    for (int i = 0; i < child_count; ++i) {
      rviz_common::properties::Property * child = childAt(i);
      if (child) {
        child->load(config.mapGetChild(child->getName()));
      }
    }
  }
};

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

std::string timeSourceName(const TimeSource source)
{
  switch (source) {
    case TimeSource::ReceiveTime:
      return "Receive Time";
    case TimeSource::HeaderStamp:
      return "Message Header Stamp";
  }
  return "Receive Time";
}

TimeSource timeSourceFromName(const std::string & name)
{
  if (name == "Message Header Stamp") {
    return TimeSource::HeaderStamp;
  }
  return TimeSource::ReceiveTime;
}

void addTimeSourceOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(timeSourceName(TimeSource::ReceiveTime));
  property->addOptionStd(timeSourceName(TimeSource::HeaderStamp));
}

std::string plotModeName(const PlotMode mode)
{
  switch (mode) {
    case PlotMode::TimeSeries:
      return "Time Series";
    case PlotMode::XY:
      return "XY";
  }
  return "Time Series";
}

PlotMode plotModeFromName(const std::string & name)
{
  if (name == "XY") {
    return PlotMode::XY;
  }
  return PlotMode::TimeSeries;
}

void addPlotModeOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(plotModeName(PlotMode::TimeSeries));
  property->addOptionStd(plotModeName(PlotMode::XY));
}

std::string xyHistoryModeName(const XYHistoryMode mode)
{
  switch (mode) {
    case XYHistoryMode::RollingTimeWindow:
      return "Rolling Time Window";
    case XYHistoryMode::AllSamples:
      return "All Samples";
  }
  return "Rolling Time Window";
}

XYHistoryMode xyHistoryModeFromName(const std::string & name)
{
  if (name == "All Samples") {
    return XYHistoryMode::AllSamples;
  }
  return XYHistoryMode::RollingTimeWindow;
}

void addXYHistoryModeOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(xyHistoryModeName(XYHistoryMode::RollingTimeWindow));
  property->addOptionStd(xyHistoryModeName(XYHistoryMode::AllSamples));
}

std::string xyAxisScaleModeName(const XYAxisScaleMode mode)
{
  switch (mode) {
    case XYAxisScaleMode::Independent:
      return "Independent";
    case XYAxisScaleMode::Equal:
      return "1:1";
  }
  return "Independent";
}

XYAxisScaleMode xyAxisScaleModeFromName(const std::string & name)
{
  if (name == "1:1") {
    return XYAxisScaleMode::Equal;
  }
  return XYAxisScaleMode::Independent;
}

void addXYAxisScaleModeOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(xyAxisScaleModeName(XYAxisScaleMode::Independent));
  property->addOptionStd(xyAxisScaleModeName(XYAxisScaleMode::Equal));
}

std::string horizontalAlignmentName(const HorizontalAlignment alignment)
{
  switch (alignment) {
    case HorizontalAlignment::Left:
      return "Left";
    case HorizontalAlignment::Center:
      return "Center";
    case HorizontalAlignment::Right:
      return "Right";
  }
  return "Right";
}

HorizontalAlignment horizontalAlignmentFromName(const std::string & name)
{
  if (name == "Left") {
    return HorizontalAlignment::Left;
  }
  if (name == "Center") {
    return HorizontalAlignment::Center;
  }
  return HorizontalAlignment::Right;
}

void addHorizontalAlignmentOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(horizontalAlignmentName(HorizontalAlignment::Left));
  property->addOptionStd(horizontalAlignmentName(HorizontalAlignment::Center));
  property->addOptionStd(horizontalAlignmentName(HorizontalAlignment::Right));
}

std::string verticalAlignmentName(const VerticalAlignment alignment)
{
  switch (alignment) {
    case VerticalAlignment::Top:
      return "Top";
    case VerticalAlignment::Center:
      return "Center";
    case VerticalAlignment::Bottom:
      return "Bottom";
  }
  return "Top";
}

VerticalAlignment verticalAlignmentFromName(const std::string & name)
{
  if (name == "Center") {
    return VerticalAlignment::Center;
  }
  if (name == "Bottom") {
    return VerticalAlignment::Bottom;
  }
  return VerticalAlignment::Top;
}

void addVerticalAlignmentOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(verticalAlignmentName(VerticalAlignment::Top));
  property->addOptionStd(verticalAlignmentName(VerticalAlignment::Center));
  property->addOptionStd(verticalAlignmentName(VerticalAlignment::Bottom));
}

rviz_2d_overlay_plugins::HorizontalAlignment toOverlayHorizontalAlignment(
  const HorizontalAlignment alignment)
{
  switch (alignment) {
    case HorizontalAlignment::Left:
      return rviz_2d_overlay_plugins::HorizontalAlignment::LEFT;
    case HorizontalAlignment::Center:
      return rviz_2d_overlay_plugins::HorizontalAlignment::CENTER;
    case HorizontalAlignment::Right:
      return rviz_2d_overlay_plugins::HorizontalAlignment::RIGHT;
  }
  return rviz_2d_overlay_plugins::HorizontalAlignment::RIGHT;
}

rviz_2d_overlay_plugins::VerticalAlignment toOverlayVerticalAlignment(
  const VerticalAlignment alignment)
{
  switch (alignment) {
    case VerticalAlignment::Top:
      return rviz_2d_overlay_plugins::VerticalAlignment::TOP;
    case VerticalAlignment::Center:
      return rviz_2d_overlay_plugins::VerticalAlignment::CENTER;
    case VerticalAlignment::Bottom:
      return rviz_2d_overlay_plugins::VerticalAlignment::BOTTOM;
  }
  return rviz_2d_overlay_plugins::VerticalAlignment::TOP;
}

constexpr const char * kNoSeriesAction = "None";

void addSeriesActionOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(kNoSeriesAction);
  property->addOptionStd("Duplicate");
  property->addOptionStd("Delete");
  property->addOptionStd("Move Up");
  property->addOptionStd("Move Down");
}

std::string legendPositionName(const LegendPosition position)
{
  switch (position) {
    case LegendPosition::TopLeft:
      return "Top Left";
    case LegendPosition::TopRight:
      return "Top Right";
    case LegendPosition::BottomLeft:
      return "Bottom Left";
    case LegendPosition::BottomRight:
      return "Bottom Right";
  }
  return "Top Left";
}

LegendPosition legendPositionFromName(const std::string & name)
{
  if (name == "Top Right") {
    return LegendPosition::TopRight;
  }
  if (name == "Bottom Left") {
    return LegendPosition::BottomLeft;
  }
  if (name == "Bottom Right") {
    return LegendPosition::BottomRight;
  }
  return LegendPosition::TopLeft;
}

void addLegendPositionOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(legendPositionName(LegendPosition::TopLeft));
  property->addOptionStd(legendPositionName(LegendPosition::TopRight));
  property->addOptionStd(legendPositionName(LegendPosition::BottomLeft));
  property->addOptionStd(legendPositionName(LegendPosition::BottomRight));
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

  plot_mode_property_ = new rviz_common::properties::EnumProperty(
    "Plot Mode", QString::fromStdString(plotModeName(PlotMode::TimeSeries)),
    "Choose time-series or same-topic XY plotting.",
    this, SLOT(onPlotModeChanged()), this);
  addPlotModeOptions(plot_mode_property_);

  series_root_property_ = new rviz_common::properties::Property(
    "Series", QVariant(), "Topic field series to draw.", this);
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
    SLOT(onConfigPropertyChanged()), this);
  refresh_rate_property_->setMin(1.0F);

  x_axis_root_property_ = new rviz_common::properties::Property(
    "X Axis", QVariant(), "X-axis scale settings for XY mode.", this);
  x_auto_scale_property_ = new rviz_common::properties::BoolProperty(
    "Auto Scale", true, "Automatically fit XY x-axis values.",
    x_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  x_min_property_ = new rviz_common::properties::FloatProperty(
    "X Min", -1.0F, "Fixed x-axis minimum when auto scale is disabled.",
    x_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  x_max_property_ = new rviz_common::properties::FloatProperty(
    "X Max", 1.0F, "Fixed x-axis maximum when auto scale is disabled.",
    x_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  x_axis_scale_property_ = new rviz_common::properties::EnumProperty(
    "Axis Scale",
    QString::fromStdString(xyAxisScaleModeName(XYAxisScaleMode::Independent)),
    "XY axis scale relationship.", x_axis_root_property_,
    SLOT(onConfigPropertyChanged()), this);
  addXYAxisScaleModeOptions(x_axis_scale_property_);

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
  horizontal_alignment_property_ = new rviz_common::properties::EnumProperty(
    "Horizontal Alignment",
    QString::fromStdString(horizontalAlignmentName(HorizontalAlignment::Right)),
    "Horizontal screen anchor used by X Offset.",
    layout_root_property_, SLOT(onConfigPropertyChanged()), this);
  addHorizontalAlignmentOptions(horizontal_alignment_property_);
  vertical_alignment_property_ = new rviz_common::properties::EnumProperty(
    "Vertical Alignment",
    QString::fromStdString(verticalAlignmentName(VerticalAlignment::Top)),
    "Vertical screen anchor used by Y Offset.",
    layout_root_property_, SLOT(onConfigPropertyChanged()), this);
  addVerticalAlignmentOptions(vertical_alignment_property_);

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
}

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

void Plot2DDisplay::onRenderPropertyChanged()
{
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

void Plot2DDisplay::onSeriesActionChanged()
{
  auto * action_property =
    qobject_cast<rviz_common::properties::EnumProperty *>(sender());
  if (!action_property) {
    return;
  }

  const auto property_it = std::find_if(
    series_properties_.begin(), series_properties_.end(),
    [action_property](const SeriesPropertySet & properties) {
      return properties.action == action_property;
    });
  if (property_it == series_properties_.end()) {
    return;
  }

  const std::string action = action_property->getStdString();
  if (action == kNoSeriesAction) {
    return;
  }

  std::vector<SeriesConfig> series = seriesConfigFromProperties_();
  const std::size_t index = static_cast<std::size_t>(
    std::distance(series_properties_.begin(), property_it));
  bool changed = false;

  if (action == "Duplicate" && index < series.size() && series.size() < 12U) {
    SeriesConfig copy = series[index];
    copy.label = copy.label.empty() ? "Series Copy" : copy.label + " Copy";
    series.insert(series.begin() + static_cast<std::ptrdiff_t>(index + 1), copy);
    changed = true;
  } else if (action == "Delete" && index < series.size() && series.size() > 1U) {
    series.erase(series.begin() + static_cast<std::ptrdiff_t>(index));
    changed = true;
  } else if (action == "Move Up" && index > 0U && index < series.size()) {
    std::swap(series[index - 1U], series[index]);
    changed = true;
  } else if (action == "Move Down" && index + 1U < series.size()) {
    std::swap(series[index], series[index + 1U]);
    changed = true;
  }

  if (!changed) {
    const QSignalBlocker blocker(action_property);
    action_property->setValue(kNoSeriesAction);
    return;
  }

  QTimer::singleShot(
    0,
    this,
    [this, series = std::move(series)]() mutable {
      replaceSeriesProperties_(series);
      onConfigPropertyChanged();
    });
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
    config.x_field = properties.x_field ? properties.x_field->getStdString() : "";
    config.y_field = properties.y_field ? properties.y_field->getStdString() : "";
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
    properties.action = new rviz_common::properties::EnumProperty(
      "Action", kNoSeriesAction, "Duplicate, delete, or reorder this series.",
      properties.root, SLOT(onSeriesActionChanged()), this);
    addSeriesActionOptions(properties.action);
    properties.enabled = new rviz_common::properties::BoolProperty(
      "Enabled", value.enabled, "Enable this series.", properties.root,
      SLOT(onConfigPropertyChanged()), this);
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
  updateModePropertyVisibility_();
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
    properties.root = new ChildOnlyGroupProperty(
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
  if (context_) {
    context_->queueRender();
  }
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
  settings.background_color.setAlpha(190);
  settings.axis_color = axis_color_property_->getColor();
  settings.axis_color.setAlpha(230);
  settings.grid_color = grid_color_property_->getColor();
  settings.grid_color.setAlpha(80);
  settings.text_color = text_color_property_->getColor();
  settings.text_color.setAlpha(235);
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
    toOverlayHorizontalAlignment(config.layout.horizontal_alignment),
    toOverlayVerticalAlignment(config.layout.vertical_alignment));
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
