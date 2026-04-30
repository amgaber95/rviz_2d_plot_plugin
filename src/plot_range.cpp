// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_range.hpp"

#include <algorithm>
#include <cmath>

namespace rviz_2d_plot_plugin
{

double PlotRange::span() const
{
  return max - min;
}

bool PlotRange::isValid() const
{
  return std::isfinite(min) && std::isfinite(max) && min < max;
}

PlotRange makeFixedRange(const double min, const double max, const double fallback_span)
{
  const PlotRange range{min, max};
  if (range.isValid()) {
    return range;
  }

  const double center = std::isfinite(min) ? min : 0.0;
  const double span = std::max(std::abs(fallback_span), 1.0);
  return PlotRange{center - span * 0.5, center + span * 0.5};
}

PlotRange makeAutoRange(
  const std::vector<PlotSample> & samples,
  const double padding_fraction,
  const double minimum_span)
{
  if (samples.empty()) {
    return PlotRange{-1.0, 1.0};
  }

  auto minmax = std::minmax_element(
    samples.begin(), samples.end(),
    [](const PlotSample & lhs, const PlotSample & rhs) {
      return lhs.value < rhs.value;
    });

  const double min_value = minmax.first->value;
  const double max_value = minmax.second->value;
  const double raw_span = max_value - min_value;

  if (!std::isfinite(min_value) || !std::isfinite(max_value)) {
    return PlotRange{-1.0, 1.0};
  }

  if (raw_span <= 0.0) {
    const double span = std::max(std::abs(minimum_span), 1.0);
    return PlotRange{min_value - span * 0.5, max_value + span * 0.5};
  }

  const double padding = raw_span * std::max(padding_fraction, 0.0);
  return PlotRange{min_value - padding, max_value + padding};
}

}  // namespace rviz_2d_plot_plugin
