// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_path_resolver.hpp"

namespace rviz_2d_plot_plugin
{
namespace
{

bool hasMalformedArraySyntax(const std::string & path)
{
  // Allow well-formed `name[N]` segments; reject bare unmatched brackets.
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (path[i] == '[') {
      const std::size_t rb = path.find(']', i + 1);
      if (rb == std::string::npos) {
        return true;  // unmatched '['
      }
      // Ensure everything between brackets is a non-empty digit sequence.
      if (rb == i + 1) {
        return true;  // "[]" — no index
      }
      for (std::size_t j = i + 1; j < rb; ++j) {
        if (!std::isdigit(static_cast<unsigned char>(path[j]))) {
          return true;
        }
      }
      i = rb;
    } else if (path[i] == ']') {
      return true;  // unmatched ']'
    }
  }
  return false;
}

bool isTopicPrefix(const std::string & path, const std::string & topic)
{
  if (path == topic) {
    return true;
  }
  return path.size() > topic.size() &&
         path.compare(0, topic.size(), topic) == 0 &&
         path[topic.size()] == '/';
}

PlotPathResolution resolveTopicTypes(
  const std::string & topic,
  const std::vector<std::string> & types)
{
  PlotPathResolution result;
  result.topic = topic;

  if (types.size() != 1U) {
    result.status = PlotPathStatus::AmbiguousTopicType;
    result.message = "Topic has multiple visible types";
    return result;
  }

  result.type = types.front();
  return result;
}

}  // namespace

std::vector<std::string> splitFieldPath(const std::string & field_path)
{
  std::vector<std::string> segments;
  std::size_t start = 0;
  while (start < field_path.size()) {
    const std::size_t slash = field_path.find('/', start);
    const std::size_t end = slash == std::string::npos ? field_path.size() : slash;
    if (end > start) {
      segments.push_back(field_path.substr(start, end - start));
    }
    if (slash == std::string::npos) {
      break;
    }
    start = slash + 1;
  }
  return segments;
}

PlotPathResolution resolvePlotPath(
  const std::string & raw_path,
  const TopicTypeMap & topics)
{
  PlotPathResolution result;

  if (raw_path.empty()) {
    result.message = "Path is empty";
    return result;
  }
  if (raw_path.front() != '/') {
    result.status = PlotPathStatus::InvalidPath;
    result.message = "Path must start with /";
    return result;
  }
  if (hasMalformedArraySyntax(raw_path)) {
    result.status = PlotPathStatus::UnsupportedSyntax;
    result.message = "Malformed array index syntax (expected name[N])";
    return result;
  }

  std::string best_topic;
  for (const auto & entry : topics) {
    if (isTopicPrefix(raw_path, entry.first) && entry.first.size() > best_topic.size()) {
      best_topic = entry.first;
    }
  }

  if (best_topic.empty()) {
    result.status = PlotPathStatus::WaitingForTopic;
    result.message = "Waiting for topic";
    return result;
  }

  result = resolveTopicTypes(best_topic, topics.at(best_topic));
  if (result.status == PlotPathStatus::AmbiguousTopicType) {
    return result;
  }

  const std::string field_path = raw_path.size() == best_topic.size() ?
    std::string{} : raw_path.substr(best_topic.size() + 1);
  result.field_segments = splitFieldPath(field_path);
  if (result.field_segments.empty()) {
    result.status = PlotPathStatus::MissingFieldPath;
    result.message = "Path must include a field after the topic";
    return result;
  }

  result.status = PlotPathStatus::Ok;
  return result;
}

PlotPathResolution resolveTopicFieldPath(
  const std::string & topic,
  const std::string & field_path,
  const TopicTypeMap & topics)
{
  PlotPathResolution result;
  if (topic.empty() || topic.front() != '/') {
    result.status = topic.empty() ? PlotPathStatus::EmptyPath : PlotPathStatus::InvalidPath;
    result.message = topic.empty() ? "Topic is empty" : "Topic must start with /";
    return result;
  }
  if (hasMalformedArraySyntax(field_path)) {
    result.status = PlotPathStatus::UnsupportedSyntax;
    result.message = "Malformed array index syntax (expected name[N])";
    return result;
  }

  const auto topic_it = topics.find(topic);
  if (topic_it == topics.end()) {
    result.status = PlotPathStatus::WaitingForTopic;
    result.topic = topic;
    result.message = "Waiting for topic";
    return result;
  }

  result = resolveTopicTypes(topic, topic_it->second);
  if (result.status == PlotPathStatus::AmbiguousTopicType) {
    return result;
  }

  result.field_segments = splitFieldPath(field_path);
  if (result.field_segments.empty()) {
    result.status = PlotPathStatus::MissingFieldPath;
    result.message = "Path must include a field after the topic";
    return result;
  }

  result.status = PlotPathStatus::Ok;
  return result;
}

}  // namespace rviz_2d_plot_plugin
