// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONFIG_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONFIG_HPP_

#include <string>
#include <vector>

namespace rviz_2d_plot_plugin
{

enum class LineStyle
{
  Solid,
  Dash,
  Dot,
  DashDot,
};

enum class PlotStyle
{
  Line,
  Step,
  Points,
};

enum class AxisScaleMode
{
  Auto,
  Fixed,
};

enum class XAxisMode
{
  Time,
  Field,
};

enum class PlotMode
{
  TimeSeries,
  XY,
};

enum class XYHistoryMode
{
  RollingTimeWindow,
  AllSamples,
};

enum class XYAxisScaleMode
{
  Independent,
  Equal,
};

enum class TimeSource
{
  ReceiveTime,
  HeaderStamp,
};

enum class HorizontalAlignment
{
  Left,
  Center,
  Right,
};

enum class VerticalAlignment
{
  Top,
  Center,
  Bottom,
};

/// Y-axis range and padding settings.
struct AxisConfig
{
  AxisScaleMode scale_mode{AxisScaleMode::Auto};
  double fixed_min{-1.0};
  double fixed_max{1.0};
  double padding_fraction{0.08};

  bool hasValidFixedRange() const;
  void repairFixedRange(double fallback_span = 1.0);
};

/// X-axis range, source, and XY aspect settings.
struct XAxisConfig
{
  XAxisMode mode{XAxisMode::Time};
  AxisScaleMode scale_mode{AxisScaleMode::Auto};
  XYAxisScaleMode axis_scale_mode{XYAxisScaleMode::Independent};
  double fixed_min{-1.0};
  double fixed_max{1.0};
  double padding_fraction{0.08};

  bool hasValidFixedRange() const;
  void repairFixedRange(double fallback_span = 1.0);
};

/// Time window and sample timestamp policy for rolling plots.
struct TimeConfig
{
  double window_seconds{30.0};
  double refresh_rate_hz{20.0};
  bool paused{false};
  TimeSource source{TimeSource::ReceiveTime};
  XYHistoryMode xy_history_mode{XYHistoryMode::RollingTimeWindow};

  void repair();
};

/// Overlay size and screen placement settings.
struct LayoutConfig
{
  int width{360};
  int height{220};
  int x_offset{10};
  int y_offset{10};
  HorizontalAlignment horizontal_alignment{HorizontalAlignment::Right};
  VerticalAlignment vertical_alignment{VerticalAlignment::Top};

  void repair();
};

/// RGB color stored without a Qt dependency.
struct SeriesColor
{
  int red{80};
  int green{170};
  int blue{255};

  void repair();
};

/// User configuration for one plotted numeric field.
struct SeriesConfig
{
  bool enabled{true};
  std::string topic;
  std::string x_field;
  std::string y_field;
  std::string field;
  std::string label{"Series"};
  std::string unit;
  SeriesColor color;
  double line_width{2.0};
  double line_alpha{1.0};
  LineStyle line_style{LineStyle::Solid};
  PlotStyle plot_style{PlotStyle::Line};
  double value_scale{1.0};
  double value_offset{0.0};

  void repair();
};

/// User configuration for one horizontal reference line or tolerance band.
struct ReferenceConfig
{
  bool enabled{true};
  double value{0.0};
  double tolerance{0.0};
  std::string label;
  SeriesColor color{255, 180, 60};
  double alpha{1.0};
  double line_width{1.2};
  LineStyle line_style{LineStyle::Solid};

  void repair();
};

/// Complete display configuration assembled from RViz properties.
struct Plot2DConfig
{
  std::vector<SeriesConfig> series{SeriesConfig{}};
  std::vector<ReferenceConfig> references;
  PlotMode plot_mode{PlotMode::TimeSeries};
  XAxisConfig x_axis;
  AxisConfig y_axis;
  TimeConfig time;
  LayoutConfig layout;

  void repair();
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONFIG_HPP_
