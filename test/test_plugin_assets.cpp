// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string readFile(const std::string & path)
{
  std::ifstream stream(path);
  if (!stream) {
    return {};
  }

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

bool fileExists(const std::string & path)
{
  std::ifstream stream(path);
  return stream.good();
}

}  // namespace

TEST(PluginAssets, PluginDescriptionUsesStableClassId)
{
  const std::string xml = readFile(std::string(PROJECT_SOURCE_DIR) + "/plugin_description.xml");

  EXPECT_NE(xml.find("rviz_2d_plot_plugin/Plot2D"), std::string::npos);
  EXPECT_NE(xml.find("rviz_2d_plot_plugin::Plot2DDisplay"), std::string::npos);
  EXPECT_NE(xml.find("rviz_common::Display"), std::string::npos);
}

TEST(PluginAssets, PackageExportsRvizPluginDescription)
{
  const std::string xml = readFile(std::string(PROJECT_SOURCE_DIR) + "/package.xml");

  EXPECT_NE(xml.find("<rviz plugin=\"${prefix}/plugin_description.xml\"/>"), std::string::npos);
}

TEST(PluginAssets, ClassIconExists)
{
  const std::string icon_path =
    std::string(PROJECT_SOURCE_DIR) + "/icons/classes/Plot2D.svg";

  ASSERT_TRUE(fileExists(icon_path));

  const std::string svg = readFile(icon_path);
  EXPECT_NE(svg.find("<svg"), std::string::npos);
}
