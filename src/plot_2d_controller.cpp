// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"

#include <utility>

namespace rviz_2d_plot_plugin
{
namespace
{

PlotControllerStatus statusFromPathStatus(const PlotPathStatus status)
{
  switch (status) {
    case PlotPathStatus::Ok:
      return PlotControllerStatus::Ok;
    case PlotPathStatus::EmptyPath:
      return PlotControllerStatus::EmptySelection;
    case PlotPathStatus::InvalidPath:
      return PlotControllerStatus::InvalidPath;
    case PlotPathStatus::UnsupportedSyntax:
      return PlotControllerStatus::UnsupportedPathSyntax;
    case PlotPathStatus::WaitingForTopic:
      return PlotControllerStatus::WaitingForTopic;
    case PlotPathStatus::MissingFieldPath:
      return PlotControllerStatus::MissingFieldPath;
    case PlotPathStatus::AmbiguousTopicType:
      return PlotControllerStatus::AmbiguousTopicType;
  }
  return PlotControllerStatus::InvalidPath;
}

}  // namespace

void Plot2DController::configure(
  Plot2DConfig config,
  const TopicTypeMap & topics)
{
  config_ = std::move(config);
  config_.repair();
  state_ = Plot2DControllerState{};
  extractor_.reset();

  const SeriesConfig & series = config_.series.front();
  if (!series.enabled) {
    state_.status = PlotControllerStatus::Disabled;
    state_.message = "Series is disabled";
    return;
  }

  const PlotPathResolution resolution =
    resolveTopicFieldPath(series.topic, series.field, topics);
  state_.topic = resolution.topic;
  state_.type = resolution.type;

  if (resolution.status != PlotPathStatus::Ok) {
    setResolutionError(resolution);
    return;
  }

  extractor_ = std::make_unique<GenericFieldExtractor>(
    resolution.type, resolution.field_segments);
  if (!extractor_->ready()) {
    state_.status = PlotControllerStatus::ExtractorError;
    state_.message = extractor_->error();
    return;
  }

  state_.status = PlotControllerStatus::Ok;
}

bool Plot2DController::appendSerializedMessage(
  const std::string & topic,
  const rclcpp::SerializedMessage & serialized,
  const double receive_time)
{
  if (topic != state_.topic || config_.time.paused ||
    state_.status != PlotControllerStatus::Ok || !extractor_)
  {
    return false;
  }

  const FieldExtractionResult result = extractor_->extract(serialized);
  if (result.status != FieldExtractionStatus::Ok || !result.value.has_value()) {
    state_.status = PlotControllerStatus::ExtractionError;
    state_.message = result.message;
    return false;
  }

  state_.samples.append(receive_time, result.value.value());
  state_.samples.pruneToWindow(receive_time, config_.time.window_seconds);
  state_.latest_value = result.value;
  return true;
}

void Plot2DController::clearHistory()
{
  state_.samples.clear();
  state_.latest_value.reset();
}

const Plot2DConfig & Plot2DController::config() const
{
  return config_;
}

const Plot2DControllerState & Plot2DController::state() const
{
  return state_;
}

void Plot2DController::setResolutionError(const PlotPathResolution & resolution)
{
  state_.status = statusFromPathStatus(resolution.status);
  state_.message = resolution.message;
}

}  // namespace rviz_2d_plot_plugin
