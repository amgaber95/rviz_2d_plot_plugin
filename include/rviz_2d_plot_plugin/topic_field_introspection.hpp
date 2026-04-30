// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_
#define RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_

#include <cstddef>
#include <string>
#include <vector>

namespace rviz_2d_plot_plugin
{

struct FieldPathOptions
{
  std::vector<std::string> paths;
  std::string error;
};

FieldPathOptions numericScalarFieldPathsForType(
  const std::string & message_type,
  std::size_t max_depth = 8);

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__TOPIC_FIELD_INTROSPECTION_HPP_
