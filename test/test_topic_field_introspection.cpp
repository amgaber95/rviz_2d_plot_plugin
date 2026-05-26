// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/string.hpp>

#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

using rviz_2d_plot_plugin::FieldExtractionStatus;
using rviz_2d_plot_plugin::GenericFieldExtractor;
using rviz_2d_plot_plugin::numericScalarFieldPathsForType;

namespace
{

bool containsPath(const std::vector<std::string> & paths, const std::string & path)
{
  return std::find(paths.begin(), paths.end(), path) != paths.end();
}

template<typename MessageT>
rclcpp::SerializedMessage serializeMessage(const MessageT & message)
{
  rclcpp::Serialization<MessageT> serializer;
  rclcpp::SerializedMessage serialized;
  serializer.serialize_message(&message, &serialized);
  return serialized;
}

}  // namespace

TEST(TopicFieldIntrospection, ListsNestedNumericFields)
{
  const auto options = numericScalarFieldPathsForType("geometry_msgs/msg/Twist");

  EXPECT_TRUE(options.error.empty());
  EXPECT_TRUE(containsPath(options.paths, "linear/x"));
  EXPECT_TRUE(containsPath(options.paths, "linear/y"));
  EXPECT_TRUE(containsPath(options.paths, "angular/z"));
  EXPECT_FALSE(containsPath(options.paths, "linear"));
}

TEST(TopicFieldIntrospection, ListsBoolAsNumericScalar)
{
  const auto options = numericScalarFieldPathsForType("std_msgs/msg/Bool");

  EXPECT_TRUE(options.error.empty());
  const std::vector<std::string> expected{"data"};
  EXPECT_EQ(options.paths, expected);
}

TEST(TopicFieldIntrospection, ListsIndexedPathsForNumericArrayFields)
{
  const auto options = numericScalarFieldPathsForType("std_msgs/msg/Float64MultiArray");

  EXPECT_TRUE(options.error.empty());
  // Bare name must not appear — only indexed variants.
  EXPECT_FALSE(containsPath(options.paths, "data"));
  // Dynamic sequence: indices 0..7 should be enumerated (kMaxDynamicArrayPreview = 8).
  EXPECT_TRUE(containsPath(options.paths, "data[0]"));
  EXPECT_TRUE(containsPath(options.paths, "data[7]"));
  EXPECT_FALSE(containsPath(options.paths, "data[8]"));
  // Arrays of nested message types must not be enumerated.
  EXPECT_FALSE(containsPath(options.paths, "layout/dim"));
}

TEST(TopicFieldIntrospection, ReportsTypeSupportErrors)
{
  const auto options = numericScalarFieldPathsForType("not_a_pkg/msg/Nope");

  EXPECT_TRUE(options.paths.empty());
  EXPECT_FALSE(options.error.empty());
}

TEST(TopicFieldIntrospection, ExtractsTopLevelDouble)
{
  std_msgs::msg::Float64 message;
  message.data = 4.25;
  GenericFieldExtractor extractor("std_msgs/msg/Float64", {"data"});

  ASSERT_TRUE(extractor.ready()) << extractor.error();
  const auto result = extractor.extract(serializeMessage(message));

  ASSERT_EQ(result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(result.value.has_value());
  EXPECT_DOUBLE_EQ(result.value.value(), 4.25);
}

TEST(TopicFieldIntrospection, ExtractsTopLevelInteger)
{
  std_msgs::msg::Int32 message;
  message.data = -42;
  GenericFieldExtractor extractor("std_msgs/msg/Int32", {"data"});

  ASSERT_TRUE(extractor.ready()) << extractor.error();
  const auto result = extractor.extract(serializeMessage(message));

  ASSERT_EQ(result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(result.value.has_value());
  EXPECT_DOUBLE_EQ(result.value.value(), -42.0);
}

TEST(TopicFieldIntrospection, ExtractsNestedDouble)
{
  geometry_msgs::msg::Twist message;
  message.linear.x = 1.5;
  message.angular.z = -2.75;
  GenericFieldExtractor linear_extractor(
    "geometry_msgs/msg/Twist", {"linear", "x"});
  GenericFieldExtractor angular_extractor(
    "geometry_msgs/msg/Twist", {"angular", "z"});
  const auto serialized = serializeMessage(message);

  ASSERT_TRUE(linear_extractor.ready()) << linear_extractor.error();
  ASSERT_TRUE(angular_extractor.ready()) << angular_extractor.error();
  const auto linear_result = linear_extractor.extract(serialized);
  const auto angular_result = angular_extractor.extract(serialized);

  ASSERT_EQ(linear_result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(linear_result.value.has_value());
  EXPECT_DOUBLE_EQ(linear_result.value.value(), 1.5);
  ASSERT_EQ(angular_result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(angular_result.value.has_value());
  EXPECT_DOUBLE_EQ(angular_result.value.value(), -2.75);
}

TEST(TopicFieldIntrospection, ExtractsBoolAsZeroOrOne)
{
  std_msgs::msg::Bool message;
  GenericFieldExtractor extractor("std_msgs/msg/Bool", {"data"});

  ASSERT_TRUE(extractor.ready()) << extractor.error();
  message.data = false;
  const auto false_result = extractor.extract(serializeMessage(message));
  message.data = true;
  const auto true_result = extractor.extract(serializeMessage(message));

  ASSERT_EQ(false_result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(false_result.value.has_value());
  EXPECT_DOUBLE_EQ(false_result.value.value(), 0.0);
  ASSERT_EQ(true_result.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(true_result.value.has_value());
  EXPECT_DOUBLE_EQ(true_result.value.value(), 1.0);
}

TEST(TopicFieldIntrospection, RejectsInvalidFieldPaths)
{
  std_msgs::msg::Float64 message;
  GenericFieldExtractor missing_extractor("std_msgs/msg/Float64", {"missing"});
  GenericFieldExtractor scalar_parent_extractor("std_msgs/msg/Float64", {"data", "x"});

  ASSERT_TRUE(missing_extractor.ready()) << missing_extractor.error();
  ASSERT_TRUE(scalar_parent_extractor.ready()) << scalar_parent_extractor.error();
  const auto serialized = serializeMessage(message);

  EXPECT_EQ(
    missing_extractor.extract(serialized).status,
    FieldExtractionStatus::PathError);
  EXPECT_EQ(
    scalar_parent_extractor.extract(serialized).status,
    FieldExtractionStatus::PathError);
}

TEST(TopicFieldIntrospection, RejectsUnsupportedLeafFields)
{
  std_msgs::msg::String string_message;
  string_message.data = "not numeric";
  std_msgs::msg::Float64MultiArray array_message;
  array_message.data.push_back(1.0);
  GenericFieldExtractor string_extractor("std_msgs/msg/String", {"data"});
  GenericFieldExtractor array_extractor("std_msgs/msg/Float64MultiArray", {"data"});

  ASSERT_TRUE(string_extractor.ready()) << string_extractor.error();
  ASSERT_TRUE(array_extractor.ready()) << array_extractor.error();

  EXPECT_EQ(
    string_extractor.extract(serializeMessage(string_message)).status,
    FieldExtractionStatus::UnsupportedField);
  EXPECT_EQ(
    array_extractor.extract(serializeMessage(array_message)).status,
    FieldExtractionStatus::UnsupportedField);
}

TEST(TopicFieldIntrospection, ExtractsArrayElementByIndex)
{
  std_msgs::msg::Float64MultiArray message;
  message.data = {10.0, 20.0, 30.0};
  GenericFieldExtractor extractor0("std_msgs/msg/Float64MultiArray", {"data[0]"});
  GenericFieldExtractor extractor2("std_msgs/msg/Float64MultiArray", {"data[2]"});

  ASSERT_TRUE(extractor0.ready()) << extractor0.error();
  ASSERT_TRUE(extractor2.ready()) << extractor2.error();
  const auto serialized = serializeMessage(message);

  const auto result0 = extractor0.extract(serialized);
  ASSERT_EQ(result0.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(result0.value.has_value());
  EXPECT_DOUBLE_EQ(result0.value.value(), 10.0);

  const auto result2 = extractor2.extract(serialized);
  ASSERT_EQ(result2.status, FieldExtractionStatus::Ok);
  ASSERT_TRUE(result2.value.has_value());
  EXPECT_DOUBLE_EQ(result2.value.value(), 30.0);
}

TEST(TopicFieldIntrospection, RejectsOutOfBoundsArrayIndex)
{
  std_msgs::msg::Float64MultiArray message;
  message.data = {1.0, 2.0, 3.0};
  GenericFieldExtractor extractor("std_msgs/msg/Float64MultiArray", {"data[10]"});

  ASSERT_TRUE(extractor.ready()) << extractor.error();

  EXPECT_EQ(
    extractor.extract(serializeMessage(message)).status,
    FieldExtractionStatus::PathError);
}

TEST(TopicFieldIntrospection, ReportsMissingExtractionTypeSupport)
{
  std_msgs::msg::Float64 message;
  GenericFieldExtractor extractor("not_a_pkg/msg/Nope", {"data"});

  EXPECT_FALSE(extractor.ready());
  EXPECT_FALSE(extractor.error().empty());
  EXPECT_EQ(
    extractor.extract(serializeMessage(message)).status,
    FieldExtractionStatus::TypeSupportError);
}
