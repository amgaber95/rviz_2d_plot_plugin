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
};

struct PlotRenderSettings
{
  int width{360};
  int height{220};
  double now{0.0};
  double window_seconds{30.0};
  AxisScaleMode y_scale_mode{AxisScaleMode::Auto};
  double fixed_y_min{-1.0};
  double fixed_y_max{1.0};
  double y_padding_fraction{0.08};
  QColor background_color{0, 0, 0, 190};
  QColor axis_color{230, 230, 230, 230};
  QColor grid_color{130, 130, 130, 80};
  QColor text_color{245, 245, 245, 235};
};

class Plot2DRenderer
{
public:
  QImage render(
    PlotRenderSettings settings,
    const std::vector<RenderableSeries> & series) const;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_RENDERER_HPP_
