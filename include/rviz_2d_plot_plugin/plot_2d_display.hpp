// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_

#include <rviz_common/display.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"

namespace rviz_common
{
namespace properties
{
class BoolProperty;
class ColorProperty;
class EditableEnumProperty;
class FloatProperty;
class IntProperty;
class Property;
class StringProperty;
}  // namespace properties
}  // namespace rviz_common

namespace rviz_2d_plot_plugin
{

class Plot2DDisplayTestAccessor;

class Plot2DDisplay : public rviz_common::Display
{
  Q_OBJECT

  friend class Plot2DDisplayTestAccessor;

public:
  Plot2DDisplay();
  ~Plot2DDisplay() override;

protected:
  void onInitialize() override;
  void reset() override;

private Q_SLOTS:
  void onConfigPropertyChanged();
  void onClearHistoryChanged();

private:
  Plot2DConfig configFromProperties_() const;

  rviz_common::properties::BoolProperty * pause_plot_property_{nullptr};
  rviz_common::properties::BoolProperty * clear_history_property_{nullptr};
  rviz_common::properties::Property * series_root_property_{nullptr};
  rviz_common::properties::Property * series_1_property_{nullptr};
  rviz_common::properties::BoolProperty * series_enabled_property_{nullptr};
  rviz_common::properties::EditableEnumProperty * series_topic_property_{nullptr};
  rviz_common::properties::EditableEnumProperty * series_field_property_{nullptr};
  rviz_common::properties::StringProperty * series_label_property_{nullptr};
  rviz_common::properties::Property * time_root_property_{nullptr};
  rviz_common::properties::FloatProperty * window_seconds_property_{nullptr};
  rviz_common::properties::FloatProperty * refresh_rate_property_{nullptr};
  rviz_common::properties::Property * y_axis_root_property_{nullptr};
  rviz_common::properties::BoolProperty * auto_scale_property_{nullptr};
  rviz_common::properties::FloatProperty * y_min_property_{nullptr};
  rviz_common::properties::FloatProperty * y_max_property_{nullptr};
  rviz_common::properties::Property * layout_root_property_{nullptr};
  rviz_common::properties::IntProperty * width_property_{nullptr};
  rviz_common::properties::IntProperty * height_property_{nullptr};
  rviz_common::properties::IntProperty * x_offset_property_{nullptr};
  rviz_common::properties::IntProperty * y_offset_property_{nullptr};
  rviz_common::properties::Property * style_root_property_{nullptr};
  rviz_common::properties::ColorProperty * background_color_property_{nullptr};
  rviz_common::properties::ColorProperty * axis_color_property_{nullptr};
  rviz_common::properties::ColorProperty * grid_color_property_{nullptr};
  rviz_common::properties::ColorProperty * text_color_property_{nullptr};
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
