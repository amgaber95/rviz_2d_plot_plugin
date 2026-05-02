// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_display.hpp"

#include <QColor>
#include <QVariant>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/property.hpp>
#include <rviz_common/properties/status_property.hpp>
#include <rviz_common/properties/string_property.hpp>

namespace rviz_2d_plot_plugin
{

Plot2DDisplay::Plot2DDisplay()
{
  pause_plot_property_ = new rviz_common::properties::BoolProperty(
    "Pause Plot", false, "Pause incoming sample collection and hold the plot.",
    this, SLOT(onConfigPropertyChanged()), this);
  clear_history_property_ = new rviz_common::properties::BoolProperty(
    "Clear History", false, "Clear stored samples for this plot.",
    this, SLOT(onClearHistoryChanged()), this);

  series_root_property_ = new rviz_common::properties::Property(
    "Series", QVariant(), "Topic field series to draw.", this);
  series_1_property_ = new rviz_common::properties::Property(
    "Series 1", QVariant(), "First plotted topic field.", series_root_property_);
  series_enabled_property_ = new rviz_common::properties::BoolProperty(
    "Enabled", true, "Enable this series.", series_1_property_,
    SLOT(onConfigPropertyChanged()), this);
  series_topic_property_ = new rviz_common::properties::EditableEnumProperty(
    "Topic", "", "ROS 2 topic to subscribe to.", series_1_property_,
    SLOT(onConfigPropertyChanged()), this);
  series_field_property_ = new rviz_common::properties::EditableEnumProperty(
    "Field", "", "Numeric or boolean field path inside the selected message.",
    series_1_property_, SLOT(onConfigPropertyChanged()), this);
  series_label_property_ = new rviz_common::properties::StringProperty(
    "Label", "Series", "Legend label for this series.", series_1_property_,
    SLOT(onConfigPropertyChanged()), this);

  time_root_property_ = new rviz_common::properties::Property(
    "Time", QVariant(), "Time-series history and redraw settings.", this);
  window_seconds_property_ = new rviz_common::properties::FloatProperty(
    "Window Seconds", 30.0F, "Visible rolling time window in seconds.",
    time_root_property_, SLOT(onConfigPropertyChanged()), this);
  window_seconds_property_->setMin(1.0F);
  refresh_rate_property_ = new rviz_common::properties::FloatProperty(
    "Refresh Rate", 20.0F, "Overlay redraw rate in Hz.", time_root_property_,
    SLOT(onConfigPropertyChanged()), this);
  refresh_rate_property_->setMin(1.0F);

  y_axis_root_property_ = new rviz_common::properties::Property(
    "Y Axis", QVariant(), "Vertical value axis scaling.", this);
  auto_scale_property_ = new rviz_common::properties::BoolProperty(
    "Auto Scale", true, "Automatically fit the y-axis to visible samples.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  y_min_property_ = new rviz_common::properties::FloatProperty(
    "Y Min", -1.0F, "Fixed y-axis minimum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);
  y_max_property_ = new rviz_common::properties::FloatProperty(
    "Y Max", 1.0F, "Fixed y-axis maximum when auto scale is disabled.",
    y_axis_root_property_, SLOT(onConfigPropertyChanged()), this);

  layout_root_property_ = new rviz_common::properties::Property(
    "Layout", QVariant(), "Overlay size and screen position.", this);
  width_property_ = new rviz_common::properties::IntProperty(
    "Width", 360, "Overlay width in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this, 120);
  height_property_ = new rviz_common::properties::IntProperty(
    "Height", 220, "Overlay height in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this, 80);
  x_offset_property_ = new rviz_common::properties::IntProperty(
    "X Offset", 10, "Horizontal screen offset in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this);
  y_offset_property_ = new rviz_common::properties::IntProperty(
    "Y Offset", 10, "Vertical screen offset in pixels.", layout_root_property_,
    SLOT(onConfigPropertyChanged()), this);

  style_root_property_ = new rviz_common::properties::Property(
    "Style", QVariant(), "Plot colors.", this);
  background_color_property_ = new rviz_common::properties::ColorProperty(
    "Background Color", QColor(0, 0, 0), "Plot background color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  axis_color_property_ = new rviz_common::properties::ColorProperty(
    "Axis Color", QColor(230, 230, 230), "Axis and border color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  grid_color_property_ = new rviz_common::properties::ColorProperty(
    "Grid Color", QColor(130, 130, 130), "Grid line color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
  text_color_property_ = new rviz_common::properties::ColorProperty(
    "Text Color", QColor(245, 245, 245), "Axis and legend text color.",
    style_root_property_, SLOT(onConfigPropertyChanged()), this);
}

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

void Plot2DDisplay::onConfigPropertyChanged()
{
}

void Plot2DDisplay::onClearHistoryChanged()
{
  if (!clear_history_property_ || !clear_history_property_->getBool()) {
    return;
  }
  clear_history_property_->setBool(false);
}

Plot2DConfig Plot2DDisplay::configFromProperties_() const
{
  Plot2DConfig config;
  SeriesConfig series;
  series.enabled = series_enabled_property_->getBool();
  series.topic = series_topic_property_->getStdString();
  series.field = series_field_property_->getStdString();
  series.label = series_label_property_->getStdString();
  config.series = {series};

  config.time.window_seconds = window_seconds_property_->getFloat();
  config.time.refresh_rate_hz = refresh_rate_property_->getFloat();
  config.time.paused = pause_plot_property_->getBool();

  config.y_axis.scale_mode = auto_scale_property_->getBool() ?
    AxisScaleMode::Auto : AxisScaleMode::Fixed;
  config.y_axis.fixed_min = y_min_property_->getFloat();
  config.y_axis.fixed_max = y_max_property_->getFloat();

  config.layout.width = width_property_->getInt();
  config.layout.height = height_property_->getInt();
  config.layout.x_offset = x_offset_property_->getInt();
  config.layout.y_offset = y_offset_property_->getInt();
  config.repair();
  return config;
}

}  // namespace rviz_2d_plot_plugin

PLUGINLIB_EXPORT_CLASS(rviz_2d_plot_plugin::Plot2DDisplay, rviz_common::Display)
