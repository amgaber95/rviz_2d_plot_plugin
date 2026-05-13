// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"

#include <algorithm>
#include <cmath>

namespace rviz_2d_plot_plugin
{

bool AxisConfig::hasValidFixedRange() const
{
  return std::isfinite(fixed_min) && std::isfinite(fixed_max) && fixed_min < fixed_max;
}

void AxisConfig::repairFixedRange(const double fallback_span)
{
  if (hasValidFixedRange()) {
    return;
  }

  const double center = std::isfinite(fixed_min) ? fixed_min : 0.0;
  const double span = std::max(std::abs(fallback_span), 1.0);
  fixed_min = center - span * 0.5;
  fixed_max = center + span * 0.5;
}

bool XAxisConfig::hasValidFixedRange() const
{
  return std::isfinite(fixed_min) && std::isfinite(fixed_max) && fixed_min < fixed_max;
}

void XAxisConfig::repairFixedRange(const double fallback_span)
{
  if (hasValidFixedRange()) {
    return;
  }

  const double center = std::isfinite(fixed_min) ? fixed_min : 0.0;
  const double span = std::max(std::abs(fallback_span), 1.0);
  fixed_min = center - span * 0.5;
  fixed_max = center + span * 0.5;
}

void TimeConfig::repair()
{
  if (!std::isfinite(window_seconds) || window_seconds <= 0.0) {
    window_seconds = 30.0;
  }
  if (!std::isfinite(refresh_rate_hz) || refresh_rate_hz <= 0.0) {
    refresh_rate_hz = 20.0;
  }
}

void QoSConfig::repair()
{
  if (depth < 1) {
    depth = 10;
  }
}

void LayoutConfig::repair()
{
  width = std::max(width, 120);
  height = std::max(height, 80);
}

void SeriesColor::repair()
{
  red = std::clamp(red, 0, 255);
  green = std::clamp(green, 0, 255);
  blue = std::clamp(blue, 0, 255);
}

void SeriesConfig::repair()
{
  color.repair();
  if (!std::isfinite(line_width)) {
    line_width = 2.0;
  }
  line_width = std::clamp(line_width, kMinimumLineWidth, kMaximumLineWidth);
  if (!std::isfinite(line_alpha)) {
    line_alpha = 1.0;
  }
  line_alpha = std::clamp(line_alpha, 0.0, 1.0);
  if (!std::isfinite(value_scale)) {
    value_scale = 1.0;
  }
  if (!std::isfinite(value_offset)) {
    value_offset = 0.0;
  }
}

void ReferenceConfig::repair()
{
  color.repair();
  if (!std::isfinite(value)) {
    value = 0.0;
  }
  if (!std::isfinite(tolerance) || tolerance < 0.0) {
    tolerance = 0.0;
  }
  if (!std::isfinite(alpha)) {
    alpha = 1.0;
  }
  alpha = std::clamp(alpha, 0.0, 1.0);
  if (!std::isfinite(line_width)) {
    line_width = 1.2;
  }
  line_width = std::clamp(line_width, kMinimumLineWidth, kMaximumLineWidth);
}

void Plot2DConfig::repair()
{
  if (series.empty()) {
    series.emplace_back();
  }

  x_axis.mode = plot_mode == PlotMode::XY ? XAxisMode::Field : XAxisMode::Time;

  for (SeriesConfig & item : series) {
    item.repair();
  }
  for (ReferenceConfig & reference : references) {
    reference.repair();
  }

  y_axis.repairFixedRange();
  x_axis.repairFixedRange();
  time.repair();
  qos.repair();
  layout.repair();
}

}  // namespace rviz_2d_plot_plugin
