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

void TimeConfig::repair()
{
  if (!std::isfinite(window_seconds) || window_seconds <= 0.0) {
    window_seconds = 30.0;
  }
  if (!std::isfinite(refresh_rate_hz) || refresh_rate_hz <= 0.0) {
    refresh_rate_hz = 20.0;
  }
}

void LayoutConfig::repair()
{
  width = std::max(width, 120);
  height = std::max(height, 80);
}

void Plot2DConfig::repair()
{
  if (series.empty()) {
    series.emplace_back();
  }

  y_axis.repairFixedRange();
  time.repair();
  layout.repair();
}

}  // namespace rviz_2d_plot_plugin
