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

enum class AxisScaleMode
{
  Auto,
  Fixed,
};

struct AxisConfig
{
  AxisScaleMode scale_mode{AxisScaleMode::Auto};
  double fixed_min{-1.0};
  double fixed_max{1.0};
  double padding_fraction{0.08};

  bool hasValidFixedRange() const;
  void repairFixedRange(double fallback_span = 1.0);
};

struct TimeConfig
{
  double window_seconds{30.0};
  double refresh_rate_hz{20.0};
  bool paused{false};

  void repair();
};

struct LayoutConfig
{
  int width{360};
  int height{220};
  int x_offset{10};
  int y_offset{10};

  void repair();
};

struct SeriesConfig
{
  bool enabled{true};
  std::string topic;
  std::string field;
  std::string label{"Series"};
};

struct Plot2DConfig
{
  std::vector<SeriesConfig> series{SeriesConfig{}};
  AxisConfig y_axis;
  TimeConfig time;
  LayoutConfig layout;

  void repair();
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONFIG_HPP_
