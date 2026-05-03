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

struct PlotSample
{
  double time{0.0};
  double value{0.0};
};

struct ValueRange
{
  double min{0.0};
  double max{0.0};
};

class RollingSampleBuffer
{
public:
  void append(double time, double value);
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
  std::optional<ValueRange> valueRange() const;

private:
  std::vector<PlotSample> samples_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__SAMPLE_BUFFER_HPP_
