// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/tick_generator.hpp"

#include <algorithm>
#include <cmath>

namespace rviz_2d_plot_plugin
{
namespace
{

constexpr double kTickEpsilon = 1e-9;

double normalizedZero(const double value)
{
  if (std::abs(value) < kTickEpsilon) {
    return 0.0;
  }
  return value;
}

PlotRange repairRange(const PlotRange & range)
{
  if (range.isValid()) {
    return range;
  }
  return makeFixedRange(range.min, range.max);
}

}  // namespace

double niceTickStep(const double raw_step)
{
  if (!std::isfinite(raw_step) || raw_step <= 0.0) {
    return 1.0;
  }

  const double exponent = std::floor(std::log10(raw_step));
  const double magnitude = std::pow(10.0, exponent);
  const double normalized = raw_step / magnitude;

  if (normalized <= 1.0) {
    return magnitude;
  }
  if (normalized <= 2.0) {
    return 2.0 * magnitude;
  }
  if (normalized <= 5.0) {
    return 5.0 * magnitude;
  }
  return 10.0 * magnitude;
}

TickSet generateTicks(
  PlotRange range,
  const std::size_t target_major_count,
  const std::size_t minor_ticks_per_major_interval)
{
  range = repairRange(range);
  const std::size_t target_count = std::max<std::size_t>(target_major_count, 2);
  const double step = niceTickStep(range.span() / static_cast<double>(target_count - 1));

  const double first = std::ceil((range.min - kTickEpsilon) / step) * step;
  const double last = std::floor((range.max + kTickEpsilon) / step) * step;

  TickSet ticks;
  for (double value = first; value <= last + kTickEpsilon; value += step) {
    ticks.major.push_back(normalizedZero(value));
  }

  if (minor_ticks_per_major_interval == 0 || ticks.major.size() < 2) {
    return ticks;
  }

  const double minor_step = step / static_cast<double>(minor_ticks_per_major_interval + 1);
  for (std::size_t major_index = 0; major_index + 1 < ticks.major.size(); ++major_index) {
    const double major = ticks.major[major_index];
    for (std::size_t minor_index = 1; minor_index <= minor_ticks_per_major_interval;
      ++minor_index)
    {
      const double minor = major + minor_step * static_cast<double>(minor_index);
      if (minor > range.min + kTickEpsilon && minor < range.max - kTickEpsilon) {
        ticks.minor.push_back(normalizedZero(minor));
      }
    }
  }

  return ticks;
}

}  // namespace rviz_2d_plot_plugin
