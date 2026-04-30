// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

#include <rosidl_runtime_c/message_type_support_struct.h>

#include <algorithm>
#include <exception>

#include <rclcpp/typesupport_helpers.hpp>
#include <rosidl_typesupport_introspection_cpp/field_types.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>

namespace rviz_2d_plot_plugin
{
namespace
{

using rosidl_typesupport_introspection_cpp::MessageMember;
using rosidl_typesupport_introspection_cpp::MessageMembers;

const MessageMembers * membersFromTypeSupport(
  const rosidl_message_type_support_t * type_support)
{
  if (!type_support || !type_support->data) {
    return nullptr;
  }
  return static_cast<const MessageMembers *>(type_support->data);
}

const MessageMembers * nestedMembers(const MessageMember & member)
{
  if (!member.members_) {
    return nullptr;
  }
  const rosidl_message_type_support_t * nested_type_support =
    get_message_typesupport_handle(
    member.members_,
    rosidl_typesupport_introspection_cpp::typesupport_identifier);
  return membersFromTypeSupport(nested_type_support);
}

bool isNumericScalarType(uint8_t type)
{
  return type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32 ||
         type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64;
}

void collectNumericFieldPaths(
  const MessageMembers * members,
  const std::string & prefix,
  std::size_t depth,
  std::size_t max_depth,
  std::vector<std::string> & paths)
{
  if (!members || depth > max_depth) {
    return;
  }

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const MessageMember & member = members->members_[i];
    if (member.is_array_) {
      continue;
    }

    const std::string path = prefix.empty() ?
      std::string(member.name_) : prefix + "/" + member.name_;
    if (isNumericScalarType(member.type_id_)) {
      paths.push_back(path);
      continue;
    }

    if (member.type_id_ ==
      rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE)
    {
      collectNumericFieldPaths(
        nestedMembers(member), path, depth + 1, max_depth, paths);
    }
  }
}

}  // namespace

FieldPathOptions numericScalarFieldPathsForType(
  const std::string & message_type,
  std::size_t max_depth)
{
  FieldPathOptions options;
  try {
    auto introspection_type_support_library = rclcpp::get_typesupport_library(
      message_type, "rosidl_typesupport_introspection_cpp");
    const auto * introspection_type_support = rclcpp::get_typesupport_handle(
      message_type,
      "rosidl_typesupport_introspection_cpp",
      *introspection_type_support_library);
    const MessageMembers * members =
      membersFromTypeSupport(introspection_type_support);
    if (!members) {
      options.error = "Introspection message members are unavailable";
      return options;
    }

    collectNumericFieldPaths(
      members, std::string{}, 0, max_depth, options.paths);
    std::sort(options.paths.begin(), options.paths.end());
  } catch (const std::exception & e) {
    options.error = e.what();
  }
  return options;
}

}  // namespace rviz_2d_plot_plugin
