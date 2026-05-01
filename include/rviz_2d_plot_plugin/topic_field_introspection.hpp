// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_
#define RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <rclcpp/serialized_message.hpp>

namespace rviz_2d_plot_plugin
{

enum class FieldExtractionStatus
{
  Ok,
  TypeSupportError,
  DeserializeError,
  PathError,
  UnsupportedField
};

struct FieldExtractionResult
{
  FieldExtractionStatus status{FieldExtractionStatus::TypeSupportError};
  std::optional<double> value;
  std::string message;
};

struct FieldPathOptions
{
  std::vector<std::string> paths;
  std::string error;
};

class GenericFieldExtractor
{
public:
  GenericFieldExtractor(
    std::string message_type,
    std::vector<std::string> field_segments);
  ~GenericFieldExtractor();

  GenericFieldExtractor(GenericFieldExtractor && other) noexcept;
  GenericFieldExtractor & operator=(GenericFieldExtractor && other) noexcept;

  GenericFieldExtractor(const GenericFieldExtractor &) = delete;
  GenericFieldExtractor & operator=(const GenericFieldExtractor &) = delete;

  bool ready() const;
  const std::string & error() const;
  FieldExtractionResult extract(
    const rclcpp::SerializedMessage & serialized) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

FieldPathOptions numericScalarFieldPathsForType(
  const std::string & message_type,
  std::size_t max_depth = 8);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_
