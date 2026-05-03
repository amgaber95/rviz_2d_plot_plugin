// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/sample_buffer.hpp"

#include <algorithm>
#include <cmath>

namespace rviz_2d_plot_plugin
{

void RollingSampleBuffer::append(const double time, const double value)
{
  samples_.push_back(PlotSample{time, value});
}

void RollingSampleBuffer::pruneBefore(const double minimum_time)
{
  const auto first_kept = std::remove_if(
    samples_.begin(), samples_.end(),
    [minimum_time](const PlotSample & sample) {
      return sample.time < minimum_time;
    });
  samples_.erase(first_kept, samples_.end());
}

void RollingSampleBuffer::pruneToWindow(const double latest_time, const double window_seconds)
{
  pruneBefore(latest_time - std::max(window_seconds, 0.0));
}

void RollingSampleBuffer::rewriteValuesForTransformChange(
  const double old_scale,
  const double old_offset,
  const double new_scale,
  const double new_offset)
{
  if (!std::isfinite(old_scale) || !std::isfinite(old_offset) ||
    !std::isfinite(new_scale) || !std::isfinite(new_offset) ||
    old_scale == 0.0)
  {
    clear();
    return;
  }

  for (PlotSample & sample : samples_) {
    const double raw_value = (sample.value - old_offset) / old_scale;
    sample.value = raw_value * new_scale + new_offset;
  }
}

void RollingSampleBuffer::clear()
{
  samples_.clear();
}

bool RollingSampleBuffer::empty() const
{
  return samples_.empty();
}

std::size_t RollingSampleBuffer::size() const
{
  return samples_.size();
}

const std::vector<PlotSample> & RollingSampleBuffer::samples() const
{
  return samples_;
}

std::optional<PlotSample> RollingSampleBuffer::latest() const
{
  if (samples_.empty()) {
    return std::nullopt;
  }
  return samples_.back();
}

std::optional<ValueRange> RollingSampleBuffer::valueRange() const
{
  if (samples_.empty()) {
    return std::nullopt;
  }

  ValueRange range{samples_.front().value, samples_.front().value};
  for (const PlotSample & sample : samples_) {
    range.min = std::min(range.min, sample.value);
    range.max = std::max(range.max, sample.value);
  }
  return range;
}

}  // namespace rviz_2d_plot_plugin
