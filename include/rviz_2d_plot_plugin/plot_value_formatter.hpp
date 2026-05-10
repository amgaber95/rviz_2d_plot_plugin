// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_VALUE_FORMATTER_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_VALUE_FORMATTER_HPP_

#include <string>

namespace rviz_2d_plot_plugin
{

std::string formatPlotValue(double value);
std::string formatAxisTickValue(double value, double step);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_VALUE_FORMATTER_HPP_
