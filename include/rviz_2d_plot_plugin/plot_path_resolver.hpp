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

struct PlotPathResolution
{
  PlotPathStatus status{PlotPathStatus::EmptyPath};
  std::string topic;
  std::string type;
  std::vector<std::string> field_segments;
  std::string message;
};

std::vector<std::string> splitFieldPath(const std::string & field_path);

PlotPathResolution resolvePlotPath(
  const std::string & raw_path,
  const TopicTypeMap & topics);

PlotPathResolution resolveTopicFieldPath(
  const std::string & topic,
  const std::string & field_path,
  const TopicTypeMap & topics);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_PATH_RESOLVER_HPP_
