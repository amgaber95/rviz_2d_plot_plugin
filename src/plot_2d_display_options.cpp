// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "plot_2d_display_options.hpp"

#include <algorithm>
#include <cmath>

namespace rviz_2d_plot_plugin
{

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

std::string qosReliabilityName(const QoSReliability reliability)
{
  switch (reliability) {
    case QoSReliability::SystemDefault:
      return "System Default";
    case QoSReliability::Reliable:
      return "Reliable";
    case QoSReliability::BestEffort:
      return "Best Effort";
  }
  return "Reliable";
}

QoSReliability qosReliabilityFromName(const std::string & name)
{
  if (name == "System Default") {
    return QoSReliability::SystemDefault;
  }
  if (name == "Best Effort") {
    return QoSReliability::BestEffort;
  }
  return QoSReliability::Reliable;
}

void addQoSReliabilityOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(qosReliabilityName(QoSReliability::SystemDefault));
  property->addOptionStd(qosReliabilityName(QoSReliability::Reliable));
  property->addOptionStd(qosReliabilityName(QoSReliability::BestEffort));
}

std::string qosDurabilityName(const QoSDurability durability)
{
  switch (durability) {
    case QoSDurability::SystemDefault:
      return "System Default";
    case QoSDurability::Volatile:
      return "Volatile";
    case QoSDurability::TransientLocal:
      return "Transient Local";
  }
  return "Volatile";
}

QoSDurability qosDurabilityFromName(const std::string & name)
{
  if (name == "System Default") {
    return QoSDurability::SystemDefault;
  }
  if (name == "Transient Local") {
    return QoSDurability::TransientLocal;
  }
  return QoSDurability::Volatile;
}

void addQoSDurabilityOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(qosDurabilityName(QoSDurability::SystemDefault));
  property->addOptionStd(qosDurabilityName(QoSDurability::Volatile));
  property->addOptionStd(qosDurabilityName(QoSDurability::TransientLocal));
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

std::string seriesDefaultLabel(const SeriesConfig & series, const PlotMode plot_mode)
{
  if (series.topic.empty()) {
    return {};
  }
  if (plot_mode == PlotMode::XY || series.field.empty()) {
    return series.topic;
  }

  const std::string separator = series.field.front() == '/' ? "" : "/";
  return series.topic + separator + series.field;
}

std::string seriesAxisName(const SeriesAxis axis)
{
  switch (axis) {
    case SeriesAxis::Left:
      return "Left";
    case SeriesAxis::Right:
      return "Right";
  }
  return "Left";
}

SeriesAxis seriesAxisFromName(const std::string & name)
{
  if (name == "Right") {
    return SeriesAxis::Right;
  }
  return SeriesAxis::Left;
}

void addSeriesAxisOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(seriesAxisName(SeriesAxis::Left));
  property->addOptionStd(seriesAxisName(SeriesAxis::Right));
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

OverlayHorizontalAlignment toOverlayHorizontalAlignment(
  const HorizontalAlignment alignment)
{
  switch (alignment) {
    case HorizontalAlignment::Left:
      return OverlayHorizontalAlignment::Left;
    case HorizontalAlignment::Center:
      return OverlayHorizontalAlignment::Center;
    case HorizontalAlignment::Right:
      return OverlayHorizontalAlignment::Right;
  }
  return OverlayHorizontalAlignment::Right;
}

OverlayVerticalAlignment toOverlayVerticalAlignment(
  const VerticalAlignment alignment)
{
  switch (alignment) {
    case VerticalAlignment::Top:
      return OverlayVerticalAlignment::Top;
    case VerticalAlignment::Center:
      return OverlayVerticalAlignment::Center;
    case VerticalAlignment::Bottom:
      return OverlayVerticalAlignment::Bottom;
  }
  return OverlayVerticalAlignment::Top;
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

void addReferencePresetOptions(rviz_common::properties::EnumProperty * property)
{
  if (!property) {
    return;
  }
  property->addOptionStd(kNoReferencePreset);
  property->addOptionStd("Zero Line");
  property->addOptionStd("Target Value");
  property->addOptionStd("Upper Limit");
  property->addOptionStd("Lower Limit");
  property->addOptionStd("Tolerance Band");
}

std::vector<ReferenceConfig> referencePresetFromName(
  const std::string & name,
  const double preset_value,
  const double preset_tolerance)
{
  const double value = std::isfinite(preset_value) ? preset_value : 0.0;
  const double tolerance = std::isfinite(preset_tolerance) ? std::abs(preset_tolerance) : 0.0;

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

  if (name == "Target Value") {
    ReferenceConfig reference;
    reference.value = value;
    reference.label = "Target";
    reference.color = SeriesColor{80, 170, 255};
    reference.alpha = 0.9;
    reference.line_width = 1.2;
    reference.line_style = LineStyle::Solid;
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

  if (name == "Tolerance Band") {
    ReferenceConfig reference;
    reference.value = value;
    reference.tolerance = tolerance;
    reference.label = "Target";
    reference.color = SeriesColor{255, 180, 60};
    reference.alpha = 0.9;
    reference.line_width = 1.2;
    reference.line_style = LineStyle::Solid;
    return {reference};
  }

  return {};
}

}  // namespace rviz_2d_plot_plugin
