// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef PLOT_2D_DISPLAY_OPTIONS_HPP_
#define PLOT_2D_DISPLAY_OPTIONS_HPP_

#include <QColor>

#include <cstddef>
#include <string>
#include <vector>

#include <rviz_common/properties/enum_property.hpp>

#include "overlay_backend.hpp"

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_renderer.hpp"

namespace rviz_2d_plot_plugin
{

inline constexpr const char * kNoReferencePreset = "None";
inline constexpr const char * kReferenceValuePropertyName = "Y Value";
inline constexpr int kReferenceFixedPropertyCount = 5;

SeriesColor defaultSeriesColor(std::size_t index);
QColor toQColor(const SeriesColor & color);
SeriesColor toSeriesColor(const QColor & color);

std::string lineStyleName(LineStyle style);
LineStyle lineStyleFromName(const std::string & name);
void addLineStyleOptions(rviz_common::properties::EnumProperty * property);

std::string plotStyleName(PlotStyle style);
PlotStyle plotStyleFromName(const std::string & name);
void addPlotStyleOptions(rviz_common::properties::EnumProperty * property);

std::string timeSourceName(TimeSource source);
TimeSource timeSourceFromName(const std::string & name);
void addTimeSourceOptions(rviz_common::properties::EnumProperty * property);

std::string qosReliabilityName(QoSReliability reliability);
QoSReliability qosReliabilityFromName(const std::string & name);
void addQoSReliabilityOptions(rviz_common::properties::EnumProperty * property);

std::string qosDurabilityName(QoSDurability durability);
QoSDurability qosDurabilityFromName(const std::string & name);
void addQoSDurabilityOptions(rviz_common::properties::EnumProperty * property);

std::string plotModeName(PlotMode mode);
PlotMode plotModeFromName(const std::string & name);
void addPlotModeOptions(rviz_common::properties::EnumProperty * property);
std::string displaySurfaceName(DisplaySurface surface);
DisplaySurface displaySurfaceFromName(const std::string & name);
void addDisplaySurfaceOptions(rviz_common::properties::EnumProperty * property);
std::string seriesDefaultLabel(const SeriesConfig & series, PlotMode plot_mode);
std::string seriesAxisName(SeriesAxis axis);
SeriesAxis seriesAxisFromName(const std::string & name);
void addSeriesAxisOptions(rviz_common::properties::EnumProperty * property);

std::string xyHistoryModeName(XYHistoryMode mode);
XYHistoryMode xyHistoryModeFromName(const std::string & name);
void addXYHistoryModeOptions(rviz_common::properties::EnumProperty * property);

std::string xyAxisScaleModeName(XYAxisScaleMode mode);
XYAxisScaleMode xyAxisScaleModeFromName(const std::string & name);
void addXYAxisScaleModeOptions(rviz_common::properties::EnumProperty * property);

std::string horizontalAlignmentName(HorizontalAlignment alignment);
HorizontalAlignment horizontalAlignmentFromName(const std::string & name);
void addHorizontalAlignmentOptions(rviz_common::properties::EnumProperty * property);

std::string verticalAlignmentName(VerticalAlignment alignment);
VerticalAlignment verticalAlignmentFromName(const std::string & name);
void addVerticalAlignmentOptions(rviz_common::properties::EnumProperty * property);

OverlayHorizontalAlignment toOverlayHorizontalAlignment(HorizontalAlignment alignment);
OverlayVerticalAlignment toOverlayVerticalAlignment(VerticalAlignment alignment);

std::string legendPositionName(LegendPosition position);
LegendPosition legendPositionFromName(const std::string & name);
void addLegendPositionOptions(rviz_common::properties::EnumProperty * property);

void addReferencePresetOptions(rviz_common::properties::EnumProperty * property);
std::vector<ReferenceConfig> referencePresetFromName(
  const std::string & name,
  double preset_value,
  double preset_tolerance);

}  // namespace rviz_2d_plot_plugin

#endif  // PLOT_2D_DISPLAY_OPTIONS_HPP_
