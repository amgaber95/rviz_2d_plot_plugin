// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_RENDERER_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_RENDERER_HPP_

#include <QColor>
#include <QImage>

#include <string>
#include <vector>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/sample_buffer.hpp"

namespace rviz_2d_plot_plugin
{

struct RenderableSeries
{
  std::string label;
  QColor color{80, 170, 255};
  std::vector<PlotSample> samples;
  bool enabled{true};
  double line_width{2.0};
  LineStyle line_style{LineStyle::Solid};
  PlotStyle plot_style{PlotStyle::Line};
};

struct RenderableReference
{
  std::string label;
  QColor color{255, 180, 60};
  double value{0.0};
  double line_width{1.2};
  LineStyle line_style{LineStyle::Solid};
  bool enabled{true};
};

enum class LegendPosition
{
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight
};

struct PlotRenderSettings
{
  int width{360};
  int height{220};
  double now{0.0};
  double window_seconds{30.0};
  XAxisMode x_axis_mode{XAxisMode::Time};
  AxisScaleMode x_scale_mode{AxisScaleMode::Auto};
  XYAxisScaleMode xy_axis_scale_mode{XYAxisScaleMode::Independent};
  double fixed_x_min{-1.0};
  double fixed_x_max{1.0};
  double x_padding_fraction{0.08};
  AxisScaleMode y_scale_mode{AxisScaleMode::Auto};
  double fixed_y_min{-1.0};
  double fixed_y_max{1.0};
  double y_padding_fraction{0.08};
  QColor background_color{0, 0, 0, 190};
  QColor axis_color{230, 230, 230, 230};
  QColor grid_color{130, 130, 130, 80};
  QColor text_color{245, 245, 245, 235};
  int font_size{8};
  bool show_legend{true};
  bool show_latest_values{true};
  LegendPosition legend_position{LegendPosition::TopLeft};
  int legend_x_offset{4};
  int legend_y_offset{4};
  bool show_major_grid{true};
  bool show_minor_grid{true};
  int x_major_tick_count{6};
  int y_major_tick_count{5};
  int minor_grid_divisions{1};
};

class Plot2DRenderer
{
public:
  QImage render(
    PlotRenderSettings settings,
    const std::vector<RenderableSeries> & series,
    const std::vector<RenderableReference> & references = {}) const;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_RENDERER_HPP_
