// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__SAMPLE_BUFFER_HPP_
#define RVIZ_2D_PLOT_PLUGIN__SAMPLE_BUFFER_HPP_

#include <cstddef>
#include <optional>
#include <vector>

namespace rviz_2d_plot_plugin
{

/// One plotted sample with receive/header time, x-coordinate, and y-value.
struct PlotSample
{
  double time{0.0};
  double x{0.0};
  double value{0.0};

  PlotSample() = default;
  PlotSample(double sample_time, double sample_value);
  PlotSample(double sample_time, double sample_x, double sample_value);
};

/// Inclusive numeric range for samples or axis limits.
struct ValueRange
{
  double min{0.0};
  double max{0.0};
};

/// Append-only sample storage with pruning and value-transform helpers.
class RollingSampleBuffer
{
public:
  /// Append a time-series sample whose x-coordinate is time.
  void append(double time, double value);
  /// Append an XY sample with explicit x-coordinate.
  void append(double time, double x, double value);
  void pruneBefore(double minimum_time);
  void pruneToWindow(double latest_time, double window_seconds);
  void rewriteValuesForTransformChange(
    double old_scale,
    double old_offset,
    double new_scale,
    double new_offset);
  void clear();

  bool empty() const;
  std::size_t size() const;
  const std::vector<PlotSample> & samples() const;
  std::optional<PlotSample> latest() const;
  std::optional<ValueRange> xRange() const;
  std::optional<ValueRange> valueRange() const;

private:
  std::vector<PlotSample> samples_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__SAMPLE_BUFFER_HPP_
