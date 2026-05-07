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
  extractors_.clear();
  x_extractors_.clear();
  header_stamp_extractors_.clear();
  state_.series.reserve(config_.series.size());
  extractors_.reserve(config_.series.size());
  x_extractors_.reserve(config_.series.size());
  header_stamp_extractors_.reserve(config_.series.size());

  for (const SeriesConfig & series : config_.series) {
    PlotSeriesControllerState series_state;
    series_state.label = series.label;
    std::unique_ptr<GenericFieldExtractor> extractor;

    if (!series.enabled) {
      series_state.status = PlotControllerStatus::Disabled;
      series_state.message = "Series is disabled";
      state_.series.push_back(std::move(series_state));
      extractors_.push_back(nullptr);
      x_extractors_.push_back(nullptr);
      header_stamp_extractors_.push_back({});
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
    if (config_.x_axis.mode == XAxisMode::Field) {
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
    if (series_index < previous_config.series.size() &&
      series_index < previous_state.series.size() &&
      previous_state.series[series_index].status == PlotControllerStatus::Ok &&
      previous_state.series[series_index].topic == resolution.topic &&
      previous_state.series[series_index].type == resolution.type &&
      previous_config.series[series_index].field == series.field &&
      previous_config.series[series_index].x_field == series.x_field &&
      previous_config.x_axis.mode == config_.x_axis.mode &&
      previous_config.time.source == config_.time.source)
    {
      series_state.samples = previous_state.series[series_index].samples;
      series_state.samples.rewriteValuesForTransformChange(
        previous_config.series[series_index].value_scale,
        previous_config.series[series_index].value_offset,
        series.value_scale,
        series.value_offset);
      series_state.latest_value = previous_state.series[series_index].latest_value;
      if (const std::optional<PlotSample> latest = series_state.samples.latest()) {
        series_state.latest_value = latest->value;
      } else {
        series_state.latest_value.reset();
      }
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

    double x_value = sample_time;
    if (config_.x_axis.mode == XAxisMode::Field) {
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
    series.samples.pruneToWindow(sample_time, config_.time.window_seconds);
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
