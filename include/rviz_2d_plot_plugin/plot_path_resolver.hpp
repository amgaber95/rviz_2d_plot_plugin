// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_PATH_RESOLVER_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_PATH_RESOLVER_HPP_

#include <map>
#include <string>
#include <vector>

namespace rviz_2d_plot_plugin
{

/// ROS topic names mapped to the message types currently advertised for them.
using TopicTypeMap = std::map<std::string, std::vector<std::string>>;

enum class PlotPathStatus
{
  Ok,
  EmptyPath,
  InvalidPath,
  UnsupportedSyntax,
  WaitingForTopic,
  MissingFieldPath,
  AmbiguousTopicType,
};

/// Parsed topic, type, and field path for a plot source string.
struct PlotPathResolution
{
  PlotPathStatus status{PlotPathStatus::EmptyPath};
  std::string topic;
  std::string type;
  std::vector<std::string> field_segments;
  std::string message;
};

/// Split a slash-delimited message field path into member names.
std::vector<std::string> splitFieldPath(const std::string & field_path);

/// Resolve a combined path such as /topic/nested/value against visible topics.
PlotPathResolution resolvePlotPath(
  const std::string & raw_path,
  const TopicTypeMap & topics);

/// Resolve an explicit topic plus field path against visible topics.
PlotPathResolution resolveTopicFieldPath(
  const std::string & topic,
  const std::string & field_path,
  const TopicTypeMap & topics);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_PATH_RESOLVER_HPP_
