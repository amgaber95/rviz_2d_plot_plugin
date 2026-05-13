// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

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

bool containsPath(const FieldPathOptions & options, const std::string & path)
{
  return std::find(options.paths.begin(), options.paths.end(), path) != options.paths.end();
}

HeaderStampExtractor makeHeaderStampExtractor(const std::string & type)
{
  const FieldPathOptions fields = numericScalarFieldPathsForType(type, 4);
  if (!fields.error.empty() ||
    !containsPath(fields, "header/stamp/sec") ||
    !containsPath(fields, "header/stamp/nanosec"))
  {
    return {};
  }

  HeaderStampExtractor extractor;
  extractor.sec = std::make_unique<GenericFieldExtractor>(
    type, std::vector<std::string>{"header", "stamp", "sec"});
  extractor.nanosec = std::make_unique<GenericFieldExtractor>(
    type, std::vector<std::string>{"header", "stamp", "nanosec"});
  if (!extractor.sec->ready() || !extractor.nanosec->ready()) {
    return {};
  }
  return extractor;
}

std::optional<double> extractHeaderStampSeconds(
  const HeaderStampExtractor & extractor,
  const rclcpp::SerializedMessage & serialized)
{
  if (!extractor.sec || !extractor.nanosec) {
    return std::nullopt;
  }

  const FieldExtractionResult sec = extractor.sec->extract(serialized);
  const FieldExtractionResult nanosec = extractor.nanosec->extract(serialized);
  if (sec.status != FieldExtractionStatus::Ok || !sec.value.has_value() ||
    nanosec.status != FieldExtractionStatus::Ok || !nanosec.value.has_value())
  {
    return std::nullopt;
  }

  return sec.value.value() + nanosec.value.value() * 1e-9;
}

bool seriesSourceMatches(
  const Plot2DConfig & previous_config,
  const Plot2DControllerState & previous_state,
  const std::size_t previous_index,
  const Plot2DConfig & current_config,
  const SeriesConfig & current_series,
  const PlotPathResolution & current_resolution)
{
  if (previous_index >= previous_config.series.size() ||
    previous_index >= previous_state.series.size())
  {
    return false;
  }

  if (previous_config.plot_mode != current_config.plot_mode ||
    previous_config.time.source != current_config.time.source)
  {
    return false;
  }

  const SeriesConfig & previous_series = previous_config.series[previous_index];
  if (current_config.plot_mode == PlotMode::XY) {
    if (previous_series.topic != current_series.topic ||
      previous_series.x_field != current_series.x_field ||
      previous_series.y_field != current_series.y_field)
    {
      return false;
    }
  } else {
    if (previous_series.topic != current_series.topic ||
      previous_series.field != current_series.field)
    {
      return false;
    }
  }

  const PlotSeriesControllerState & previous_state_series =
    previous_state.series[previous_index];
  if (previous_state_series.status == PlotControllerStatus::Ok) {
    return previous_state_series.topic == current_resolution.topic &&
           previous_state_series.type == current_resolution.type;
  }

  return !previous_state_series.samples.empty();
}

bool seriesConfigSourceMatches(
  const Plot2DConfig & previous_config,
  const std::size_t previous_index,
  const Plot2DConfig & current_config,
  const SeriesConfig & current_series)
{
  if (previous_index >= previous_config.series.size() ||
    previous_config.plot_mode != current_config.plot_mode ||
    previous_config.time.source != current_config.time.source)
  {
    return false;
  }

  const SeriesConfig & previous_series = previous_config.series[previous_index];
  if (current_config.plot_mode == PlotMode::XY) {
    return previous_series.topic == current_series.topic &&
           previous_series.x_field == current_series.x_field &&
           previous_series.y_field == current_series.y_field;
  }
  return previous_series.topic == current_series.topic &&
         previous_series.field == current_series.field;
}

std::optional<std::size_t> matchingPreviousSeriesIndex(
  const Plot2DConfig & previous_config,
  const Plot2DControllerState & previous_state,
  const std::vector<bool> & previous_series_consumed,
  const std::size_t current_index,
  const Plot2DConfig & current_config,
  const SeriesConfig & current_series,
  const PlotPathResolution & current_resolution)
{
  if (current_index < previous_series_consumed.size() &&
    !previous_series_consumed[current_index] &&
    seriesSourceMatches(
      previous_config, previous_state, current_index, current_config, current_series,
      current_resolution))
  {
    return current_index;
  }

  for (std::size_t previous_index = 0; previous_index < previous_series_consumed.size();
    ++previous_index)
  {
    if (!previous_series_consumed[previous_index] &&
      seriesSourceMatches(
        previous_config, previous_state, previous_index, current_config, current_series,
        current_resolution))
    {
      return previous_index;
    }
  }

  return std::nullopt;
}

std::optional<std::size_t> matchingPreviousSeriesIndexByConfig(
  const Plot2DConfig & previous_config,
  const Plot2DControllerState & previous_state,
  const std::vector<bool> & previous_series_consumed,
  const std::size_t current_index,
  const Plot2DConfig & current_config,
  const SeriesConfig & current_series)
{
  if (current_index < previous_series_consumed.size() &&
    !previous_series_consumed[current_index] &&
    current_index < previous_state.series.size() &&
    !previous_state.series[current_index].samples.empty() &&
    seriesConfigSourceMatches(previous_config, current_index, current_config, current_series))
  {
    return current_index;
  }

  for (std::size_t previous_index = 0; previous_index < previous_series_consumed.size();
    ++previous_index)
  {
    if (!previous_series_consumed[previous_index] &&
      previous_index < previous_state.series.size() &&
      !previous_state.series[previous_index].samples.empty() &&
      seriesConfigSourceMatches(previous_config, previous_index, current_config, current_series))
    {
      return previous_index;
    }
  }

  return std::nullopt;
}

void preserveSamplesFromPreviousSeries(
  PlotSeriesControllerState & series_state,
  const Plot2DConfig & previous_config,
  const Plot2DControllerState & previous_state,
  std::vector<bool> & previous_series_consumed,
  const std::size_t previous_index,
  const Plot2DConfig & current_config,
  const SeriesConfig & current_series)
{
  previous_series_consumed[previous_index] = true;
  const SeriesConfig & previous_series = previous_config.series[previous_index];
  series_state.samples = previous_state.series[previous_index].samples;
  series_state.samples.rewriteValuesForTransformChange(
    previous_series.value_scale,
    previous_series.value_offset,
    current_series.value_scale,
    current_series.value_offset);

  const bool xy_mode = current_config.plot_mode == PlotMode::XY;
  if (const std::optional<PlotSample> latest = series_state.samples.latest()) {
    if (!xy_mode || current_config.time.xy_history_mode == XYHistoryMode::RollingTimeWindow) {
      series_state.samples.pruneToWindow(latest->time, current_config.time.window_seconds);
    }
  }

  if (const std::optional<PlotSample> latest = series_state.samples.latest()) {
    series_state.latest_value = latest->value;
  } else {
    series_state.latest_value.reset();
  }
}

}  // namespace

void Plot2DController::configure(
  Plot2DConfig config,
  const TopicTypeMap & topics)
{
  const Plot2DConfig previous_config = config_;
  const Plot2DControllerState previous_state = state_;

  config_ = std::move(config);
  config_.repair();
  state_ = Plot2DControllerState{};
  std::vector<bool> previous_series_consumed(previous_state.series.size(), false);
  extractors_.clear();
  x_extractors_.clear();
  header_stamp_extractors_.clear();
  state_.series.reserve(config_.series.size());
  extractors_.reserve(config_.series.size());
  x_extractors_.reserve(config_.series.size());
  header_stamp_extractors_.reserve(config_.series.size());

  for (const SeriesConfig & series : config_.series) {
    const bool xy_mode = config_.plot_mode == PlotMode::XY;
    const std::string & y_field = xy_mode ? series.y_field : series.field;
    PlotSeriesControllerState series_state;
    series_state.label = series.label;
    std::unique_ptr<GenericFieldExtractor> extractor;

    if (!series.enabled) {
      series_state.status = PlotControllerStatus::Disabled;
      series_state.message = "Series is disabled";
      const std::size_t series_index = state_.series.size();
      const std::optional<std::size_t> previous_index = matchingPreviousSeriesIndexByConfig(
        previous_config, previous_state, previous_series_consumed, series_index, config_, series);
      if (previous_index.has_value()) {
        preserveSamplesFromPreviousSeries(
          series_state, previous_config, previous_state, previous_series_consumed,
          previous_index.value(), config_, series);
      }
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      x_extractors_.push_back(nullptr);
      header_stamp_extractors_.push_back({});
      continue;
    }

    const PlotPathResolution resolution =
      resolveTopicFieldPath(series.topic, y_field, topics);
    series_state.topic = resolution.topic;
    series_state.type = resolution.type;

    if (resolution.status != PlotPathStatus::Ok) {
      setResolutionError(series_state, resolution);
      const std::size_t series_index = state_.series.size();
      const std::optional<std::size_t> previous_index = matchingPreviousSeriesIndexByConfig(
        previous_config, previous_state, previous_series_consumed, series_index, config_, series);
      if (previous_index.has_value()) {
        preserveSamplesFromPreviousSeries(
          series_state, previous_config, previous_state, previous_series_consumed,
          previous_index.value(), config_, series);
      }
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      x_extractors_.push_back(nullptr);
      header_stamp_extractors_.push_back({});
      continue;
    }

    extractor = std::make_unique<GenericFieldExtractor>(
      resolution.type, resolution.field_segments);
    if (!extractor->ready()) {
      series_state.status = PlotControllerStatus::ExtractorError;
      series_state.message = extractor->error();
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      x_extractors_.push_back(nullptr);
      header_stamp_extractors_.push_back({});
      continue;
    }

    std::unique_ptr<GenericFieldExtractor> x_extractor;
    if (xy_mode) {
      const PlotPathResolution x_resolution =
        resolveTopicFieldPath(series.topic, series.x_field, topics);
      if (x_resolution.status != PlotPathStatus::Ok ||
        x_resolution.topic != resolution.topic ||
        x_resolution.type != resolution.type)
      {
        setResolutionError(series_state, x_resolution);
        if (series_state.message.empty()) {
          series_state.message = "X field must resolve to the same topic and type";
        } else {
          series_state.message = "X field: " + series_state.message;
        }
        state_.series.push_back(std::move(series_state));
        extractors_.push_back(nullptr);
        x_extractors_.push_back(nullptr);
        header_stamp_extractors_.push_back({});
        continue;
      }

      x_extractor = std::make_unique<GenericFieldExtractor>(
        x_resolution.type, x_resolution.field_segments);
      if (!x_extractor->ready()) {
        series_state.status = PlotControllerStatus::ExtractorError;
        series_state.message = "X field: " + x_extractor->error();
        state_.series.push_back(std::move(series_state));
        extractors_.push_back(nullptr);
        x_extractors_.push_back(nullptr);
        header_stamp_extractors_.push_back({});
        continue;
      }
    }

    series_state.status = PlotControllerStatus::Ok;
    const std::size_t series_index = state_.series.size();
    const std::optional<std::size_t> previous_index = matchingPreviousSeriesIndex(
      previous_config, previous_state, previous_series_consumed, series_index, config_, series,
      resolution);
    if (previous_index.has_value()) {
      preserveSamplesFromPreviousSeries(
        series_state, previous_config, previous_state, previous_series_consumed,
        previous_index.value(), config_, series);
    }
    state_.series.push_back(std::move(series_state));
    extractors_.push_back(std::move(extractor));
    x_extractors_.push_back(std::move(x_extractor));
    header_stamp_extractors_.push_back(
      config_.time.source == TimeSource::HeaderStamp ?
      makeHeaderStampExtractor(resolution.type) : HeaderStampExtractor{});
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
    if (topic != series.topic || !extractors_[i]) {
      continue;
    }

    const FieldExtractionResult result = extractors_[i]->extract(serialized);
    if (result.status != FieldExtractionStatus::Ok || !result.value.has_value()) {
      series.status = PlotControllerStatus::ExtractionError;
      series.message = result.message;
      continue;
    }

    double sample_time = receive_time;
    if (config_.time.source == TimeSource::HeaderStamp &&
      i < header_stamp_extractors_.size())
    {
      if (const std::optional<double> header_time =
        extractHeaderStampSeconds(header_stamp_extractors_[i], serialized))
      {
        sample_time = header_time.value();
      }
    }

    const bool xy_mode = config_.plot_mode == PlotMode::XY;
    double x_value = sample_time;
    if (xy_mode) {
      if (i >= x_extractors_.size() || !x_extractors_[i]) {
        continue;
      }
      const FieldExtractionResult x_result = x_extractors_[i]->extract(serialized);
      if (x_result.status != FieldExtractionStatus::Ok || !x_result.value.has_value()) {
        series.status = PlotControllerStatus::ExtractionError;
        series.message = "X field: " + x_result.message;
        continue;
      }
      x_value = x_result.value.value();
    }

    const double transformed_value =
      result.value.value() * config_.series[i].value_scale + config_.series[i].value_offset;
    series.status = PlotControllerStatus::Ok;
    series.message.clear();
    series.samples.append(sample_time, x_value, transformed_value);
    if (!xy_mode || config_.time.xy_history_mode == XYHistoryMode::RollingTimeWindow) {
      series.samples.pruneToWindow(sample_time, config_.time.window_seconds);
    }
    series.latest_value = transformed_value;
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
