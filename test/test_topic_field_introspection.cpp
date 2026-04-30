// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "rviz_2d_plot_plugin/topic_field_introspection.hpp"

using rviz_2d_plot_plugin::numericScalarFieldPathsForType;

namespace
{

bool containsPath(const std::vector<std::string> & paths, const std::string & path)
{
  return std::find(paths.begin(), paths.end(), path) != paths.end();
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

TEST(TopicFieldIntrospection, SkipsArrayAndSequenceFields)
{
  const auto options = numericScalarFieldPathsForType("std_msgs/msg/Float64MultiArray");

  EXPECT_TRUE(options.error.empty());
  EXPECT_FALSE(containsPath(options.paths, "data"));
  EXPECT_FALSE(containsPath(options.paths, "layout/dim"));
}

TEST(TopicFieldIntrospection, ReportsTypeSupportErrors)
{
  const auto options = numericScalarFieldPathsForType("not_a_pkg/msg/Nope");

  EXPECT_TRUE(options.paths.empty());
  EXPECT_FALSE(options.error.empty());
}
