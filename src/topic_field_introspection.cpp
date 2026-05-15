// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

#include <rosidl_runtime_c/message_type_support_struct.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <utility>

#include <rclcpp/serialization.hpp>
#include <rclcpp/typesupport_helpers.hpp>
#include <rcpputils/shared_library.hpp>
#include <rosidl_runtime_cpp/message_initialization.hpp>
#include <rosidl_typesupport_introspection_cpp/field_types.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>

namespace rviz_2d_plot_plugin
{
namespace
{

using rosidl_typesupport_introspection_cpp::MessageMember;
using rosidl_typesupport_introspection_cpp::MessageMembers;

struct MessageDeleter
{
  const MessageMembers * members{nullptr};

  void operator()(void * message) const
  {
    if (!message) {
      return;
    }
    if (members) {
      members->fini_function(message);
    }
    ::operator delete(message);
  }
};

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

const MessageMember * findMember(
  const MessageMembers * members,
  const std::string & name)
{
  if (!members) {
    return nullptr;
  }
  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const MessageMember & member = members->members_[i];
    if (name == member.name_) {
      return &member;
    }
  }
  return nullptr;
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

template<typename T>
double numericValue(const void * field)
{
  return static_cast<double>(*static_cast<const T *>(field));
}

std::optional<double> readNumericScalar(const MessageMember & member, const void * field)
{
  const uint8_t type = member.type_id_;
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN ||
    type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL)
  {
    return *static_cast<const bool *>(field) ? 1.0 : 0.0;
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32) {
    return numericValue<float>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64) {
    return numericValue<double>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8) {
    return numericValue<int8_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16) {
    return numericValue<int16_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32) {
    return numericValue<int32_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64) {
    return numericValue<int64_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8 ||
    type == rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE)
  {
    return numericValue<uint8_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16) {
    return numericValue<uint16_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32) {
    return numericValue<uint32_t>(field);
  }
  if (type == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64) {
    return numericValue<uint64_t>(field);
  }
  return std::nullopt;
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

std::unique_ptr<void, MessageDeleter> makeInitializedMessage(
  const MessageMembers * members)
{
  void * message = ::operator new(members->size_of_);
  try {
    members->init_function(
      message, rosidl_runtime_cpp::MessageInitialization::ALL);
  } catch (...) {
    ::operator delete(message);
    throw;
  }
  return std::unique_ptr<void, MessageDeleter>(
    message, MessageDeleter{members});
}

FieldExtractionResult walkFieldPath(
  const MessageMembers * root_members,
  const void * root_message,
  const std::vector<std::string> & field_segments)
{
  const MessageMembers * current_members = root_members;
  const void * current_message = root_message;

  for (std::size_t i = 0; i < field_segments.size(); ++i) {
    const bool is_leaf = i + 1 == field_segments.size();
    const MessageMember * member = findMember(
      current_members, field_segments[i]);
    if (!member) {
      return FieldExtractionResult{
        FieldExtractionStatus::PathError,
        std::nullopt,
        "Field does not exist"};
    }
    if (member->is_array_) {
      return FieldExtractionResult{
        FieldExtractionStatus::UnsupportedField,
        std::nullopt,
        "Arrays and sequences are not supported"};
    }

    const auto * current_bytes = static_cast<const uint8_t *>(current_message);
    const void * field = current_bytes + member->offset_;

    if (!is_leaf) {
      if (member->type_id_ !=
        rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE)
      {
        return FieldExtractionResult{
          FieldExtractionStatus::PathError,
          std::nullopt,
          "Field path continues through a scalar"};
      }
      current_members = nestedMembers(*member);
      if (!current_members) {
        return FieldExtractionResult{
          FieldExtractionStatus::TypeSupportError,
          std::nullopt,
          "Nested message type support is unavailable"};
      }
      current_message = field;
      continue;
    }

    if (member->type_id_ ==
      rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE)
    {
      return FieldExtractionResult{
        FieldExtractionStatus::UnsupportedField,
        std::nullopt,
        "Message leaves are not numeric scalar fields"};
    }

    auto value = readNumericScalar(*member, field);
    if (!value.has_value()) {
      return FieldExtractionResult{
        FieldExtractionStatus::UnsupportedField,
        std::nullopt,
        "Leaf field is not a supported numeric scalar"};
    }

    return FieldExtractionResult{
      FieldExtractionStatus::Ok,
      value,
      std::string{}};
  }

  return FieldExtractionResult{
    FieldExtractionStatus::PathError,
    std::nullopt,
    "Field path is empty"};
}

}  // namespace

class GenericFieldExtractor::Impl
{
public:
  Impl(
    std::string message_type,
    std::vector<std::string> field_segments)
  : message_type_(std::move(message_type)),
    field_segments_(std::move(field_segments))
  {
    try {
      cpp_type_support_library_ = rclcpp::get_typesupport_library(
        message_type_, "rosidl_typesupport_cpp");
#ifdef SERVICE_TYPESUPPORT_UNAVAILABLE
      cpp_type_support_ = rclcpp::get_typesupport_handle(
        message_type_, "rosidl_typesupport_cpp", *cpp_type_support_library_);
#else
      cpp_type_support_ = rclcpp::get_message_typesupport_handle(
        message_type_, "rosidl_typesupport_cpp", *cpp_type_support_library_);
#endif
      introspection_type_support_library_ = rclcpp::get_typesupport_library(
        message_type_, "rosidl_typesupport_introspection_cpp");
#ifdef SERVICE_TYPESUPPORT_UNAVAILABLE
      introspection_type_support_ = rclcpp::get_typesupport_handle(
        message_type_,
        "rosidl_typesupport_introspection_cpp",
        *introspection_type_support_library_);
#else
      introspection_type_support_ = rclcpp::get_message_typesupport_handle(
        message_type_,
        "rosidl_typesupport_introspection_cpp",
        *introspection_type_support_library_);
#endif
      serializer_ = std::make_unique<rclcpp::SerializationBase>(
        cpp_type_support_);

      if (!membersFromTypeSupport(introspection_type_support_)) {
        error_ = "Introspection message members are unavailable";
      }
    } catch (const std::exception & e) {
      error_ = e.what();
    }
  }

  bool ready() const
  {
    return error_.empty() && cpp_type_support_ &&
           introspection_type_support_ && serializer_;
  }

  const std::string & error() const
  {
    return error_;
  }

  FieldExtractionResult extract(const rclcpp::SerializedMessage & serialized) const
  {
    if (!ready()) {
      return FieldExtractionResult{
        FieldExtractionStatus::TypeSupportError, std::nullopt, error_};
    }

    const MessageMembers * members =
      membersFromTypeSupport(introspection_type_support_);
    if (!members) {
      return FieldExtractionResult{
        FieldExtractionStatus::TypeSupportError,
        std::nullopt,
        "Introspection message members are unavailable"};
    }

    std::unique_ptr<void, MessageDeleter> message;
    try {
      message = makeInitializedMessage(members);
      serializer_->deserialize_message(&serialized, message.get());
    } catch (const std::exception & e) {
      return FieldExtractionResult{
        FieldExtractionStatus::DeserializeError,
        std::nullopt,
        e.what()};
    }

    return walkFieldPath(members, message.get(), field_segments_);
  }

private:
  std::string message_type_;
  std::vector<std::string> field_segments_;
  std::string error_;
  std::shared_ptr<rcpputils::SharedLibrary> cpp_type_support_library_;
  std::shared_ptr<rcpputils::SharedLibrary> introspection_type_support_library_;
  const rosidl_message_type_support_t * cpp_type_support_{nullptr};
  const rosidl_message_type_support_t * introspection_type_support_{nullptr};
  std::unique_ptr<rclcpp::SerializationBase> serializer_;
};

GenericFieldExtractor::GenericFieldExtractor(
  std::string message_type,
  std::vector<std::string> field_segments)
: impl_(std::make_unique<Impl>(
      std::move(message_type), std::move(field_segments)))
{
}

GenericFieldExtractor::~GenericFieldExtractor() = default;

GenericFieldExtractor::GenericFieldExtractor(
  GenericFieldExtractor && other) noexcept = default;

GenericFieldExtractor & GenericFieldExtractor::operator=(
  GenericFieldExtractor && other) noexcept = default;

bool GenericFieldExtractor::ready() const
{
  return impl_ && impl_->ready();
}

const std::string & GenericFieldExtractor::error() const
{
  if (!impl_) {
    static const std::string moved_from_error = "Extractor has no implementation";
    return moved_from_error;
  }
  return impl_->error();
}

FieldExtractionResult GenericFieldExtractor::extract(
  const rclcpp::SerializedMessage & serialized) const
{
  if (!impl_) {
    static const std::string moved_from_error = "Extractor has no implementation";
    return FieldExtractionResult{
      FieldExtractionStatus::TypeSupportError,
      std::nullopt,
      moved_from_error};
  }
  return impl_->extract(serialized);
}

FieldPathOptions numericScalarFieldPathsForType(
  const std::string & message_type,
  std::size_t max_depth)
{
  FieldPathOptions options;
  try {
    auto introspection_type_support_library = rclcpp::get_typesupport_library(
      message_type, "rosidl_typesupport_introspection_cpp");
#ifdef SERVICE_TYPESUPPORT_UNAVAILABLE
    const auto * introspection_type_support = rclcpp::get_typesupport_handle(
      message_type,
      "rosidl_typesupport_introspection_cpp",
      *introspection_type_support_library);
#else
    const auto * introspection_type_support = rclcpp::get_message_typesupport_handle(
      message_type,
      "rosidl_typesupport_introspection_cpp",
      *introspection_type_support_library);
#endif
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
