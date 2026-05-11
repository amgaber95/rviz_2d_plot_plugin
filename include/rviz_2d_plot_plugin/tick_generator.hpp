// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__TICK_GENERATOR_HPP_
#define RVIZ_2D_PLOT_PLUGIN__TICK_GENERATOR_HPP_

#include <cstddef>
#include <vector>

#include "rviz_2d_plot_plugin/plot_range.hpp"

namespace rviz_2d_plot_plugin
{

/// Major and minor tick values for one axis.
struct TickSet
{
  std::vector<double> major;
  std::vector<double> minor;
};

/// Round a raw tick interval to a readable 1/2/5 step.
double niceTickStep(double raw_step);

/// Generate readable major and minor ticks inside a range.
TickSet generateTicks(
  PlotRange range,
  std::size_t target_major_count,
  std::size_t minor_ticks_per_major_interval = 4);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__TICK_GENERATOR_HPP_
