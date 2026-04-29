// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/sample_buffer.hpp"

#include <algorithm>

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
