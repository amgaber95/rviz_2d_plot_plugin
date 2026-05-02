// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"

#include <cstddef>
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

int statusSeverity(const PlotControllerStatus status)
{
  switch (status) {
    case PlotControllerStatus::InvalidPath:
    case PlotControllerStatus::UnsupportedPathSyntax:
    case PlotControllerStatus::MissingFieldPath:
    case PlotControllerStatus::AmbiguousTopicType:
    case PlotControllerStatus::ExtractorError:
    case PlotControllerStatus::ExtractionError:
      return 3;
    case PlotControllerStatus::Disabled:
      return 0;
    case PlotControllerStatus::Ok:
      return 1;
    case PlotControllerStatus::EmptySelection:
    case PlotControllerStatus::WaitingForTopic:
      return 2;
  }
  return 3;
}

}  // namespace

void Plot2DController::configure(
  Plot2DConfig config,
  const TopicTypeMap & topics)
{
  config_ = std::move(config);
  config_.repair();
  state_ = Plot2DControllerState{};
  extractors_.clear();
  state_.series.reserve(config_.series.size());
  extractors_.reserve(config_.series.size());

  for (const SeriesConfig & series : config_.series) {
    PlotSeriesControllerState series_state;
    series_state.label = series.label;
    std::unique_ptr<GenericFieldExtractor> extractor;

    if (!series.enabled) {
      series_state.status = PlotControllerStatus::Disabled;
      series_state.message = "Series is disabled";
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      continue;
    }

    const PlotPathResolution resolution =
      resolveTopicFieldPath(series.topic, series.field, topics);
    series_state.topic = resolution.topic;
    series_state.type = resolution.type;

    if (resolution.status != PlotPathStatus::Ok) {
      setResolutionError(series_state, resolution);
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      continue;
    }

    extractor = std::make_unique<GenericFieldExtractor>(
      resolution.type, resolution.field_segments);
    if (!extractor->ready()) {
      series_state.status = PlotControllerStatus::ExtractorError;
      series_state.message = extractor->error();
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      continue;
    }

    series_state.status = PlotControllerStatus::Ok;
    state_.series.push_back(std::move(series_state));
    extractors_.push_back(std::move(extractor));
  }

  updateAggregateStatus();
}

bool Plot2DController::appendSerializedMessage(
  const std::string & topic,
  const rclcpp::SerializedMessage & serialized,
  const double receive_time)
{
  if (config_.time.paused) {
    return false;
  }

  bool appended = false;
  for (std::size_t i = 0; i < state_.series.size() && i < extractors_.size(); ++i) {
    PlotSeriesControllerState & series = state_.series[i];
    if (topic != series.topic || series.status != PlotControllerStatus::Ok ||
      !extractors_[i])
    {
      continue;
    }

    const FieldExtractionResult result = extractors_[i]->extract(serialized);
    if (result.status != FieldExtractionStatus::Ok || !result.value.has_value()) {
      series.status = PlotControllerStatus::ExtractionError;
      series.message = result.message;
      continue;
    }

    series.samples.append(receive_time, result.value.value());
    series.samples.pruneToWindow(receive_time, config_.time.window_seconds);
    series.latest_value = result.value;
    appended = true;
  }

  updateAggregateStatus();
  return appended;
}

void Plot2DController::clearHistory()
{
  for (PlotSeriesControllerState & series : state_.series) {
    series.samples.clear();
    series.latest_value.reset();
  }
}

const Plot2DConfig & Plot2DController::config() const
{
  return config_;
}

const Plot2DControllerState & Plot2DController::state() const
{
  return state_;
}

void Plot2DController::setResolutionError(
  PlotSeriesControllerState & series_state,
  const PlotPathResolution & resolution)
{
  series_state.status = statusFromPathStatus(resolution.status);
  series_state.message = resolution.message;
}

void Plot2DController::updateAggregateStatus()
{
  if (state_.series.empty()) {
    state_.status = PlotControllerStatus::EmptySelection;
    state_.message = "No series configured";
    return;
  }

  const PlotSeriesControllerState * selected = nullptr;
  for (const PlotSeriesControllerState & series : state_.series) {
    if (!selected || statusSeverity(series.status) > statusSeverity(selected->status)) {
      selected = &series;
    }
  }

  state_.status = selected->status;
  state_.message = selected->message;
}

}  // namespace rviz_2d_plot_plugin
