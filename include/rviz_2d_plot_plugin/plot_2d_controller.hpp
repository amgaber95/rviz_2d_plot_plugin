// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONTROLLER_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONTROLLER_HPP_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <rclcpp/serialized_message.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_path_resolver.hpp"
#include "rviz_2d_plot_plugin/sample_buffer.hpp"
#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

namespace rviz_2d_plot_plugin
{

enum class PlotControllerStatus
{
  Ok,
  Disabled,
  EmptySelection,
  InvalidPath,
  UnsupportedPathSyntax,
  WaitingForTopic,
  MissingFieldPath,
  AmbiguousTopicType,
  ExtractorError,
  ExtractionError,
};

struct PlotSeriesControllerState
{
  PlotControllerStatus status{PlotControllerStatus::EmptySelection};
  std::string topic;
  std::string type;
  std::string label;
  std::string message;
  RollingSampleBuffer samples;
  std::optional<double> latest_value;
};

struct Plot2DControllerState
{
  PlotControllerStatus status{PlotControllerStatus::EmptySelection};
  std::string message;
  std::vector<PlotSeriesControllerState> series;
};

class Plot2DController
{
public:
  void configure(
    Plot2DConfig config,
    const TopicTypeMap & topics);

  bool appendSerializedMessage(
    const std::string & topic,
    const rclcpp::SerializedMessage & serialized,
    double receive_time);

  void clearHistory();

  const Plot2DConfig & config() const;
  const Plot2DControllerState & state() const;

private:
  void setResolutionError(
    PlotSeriesControllerState & series_state,
    const PlotPathResolution & resolution);
  void updateAggregateStatus();

  Plot2DConfig config_;
  Plot2DControllerState state_;
  std::vector<std::unique_ptr<GenericFieldExtractor>> extractors_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_CONTROLLER_HPP_
