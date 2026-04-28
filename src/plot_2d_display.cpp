// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_display.hpp"

#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/properties/status_property.hpp>

namespace rviz_2d_plot_plugin
{

Plot2DDisplay::Plot2DDisplay() = default;

Plot2DDisplay::~Plot2DDisplay() = default;

void Plot2DDisplay::onInitialize()
{
  rviz_common::Display::onInitialize();
  setStatus(
    rviz_common::properties::StatusProperty::Ok,
    "Plugin",
    "2D plot display initialized");
}

void Plot2DDisplay::reset()
{
  rviz_common::Display::reset();
}

}  // namespace rviz_2d_plot_plugin

PLUGINLIB_EXPORT_CLASS(rviz_2d_plot_plugin::Plot2DDisplay, rviz_common::Display)
