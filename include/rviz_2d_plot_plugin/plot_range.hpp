// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_RANGE_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_RANGE_HPP_

#include <vector>

#include "rviz_2d_plot_plugin/sample_buffer.hpp"

namespace rviz_2d_plot_plugin
{

/// Numeric axis range with basic validity helpers.
struct PlotRange
{
  double min{-1.0};
  double max{1.0};

  double span() const;
  bool isValid() const;
};

/// Build a fixed range, repairing invalid limits with a fallback span.
PlotRange makeFixedRange(double min, double max, double fallback_span = 1.0);

/// Build an auto range from visible samples and padding.
PlotRange makeAutoRange(
  const std::vector<PlotSample> & samples,
  double padding_fraction,
  double minimum_span = 1.0);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_RANGE_HPP_
