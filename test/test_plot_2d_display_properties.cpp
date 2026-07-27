// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "plot_2d_display_test_helpers.hpp"

namespace
{

using rviz_2d_plot_plugin::AxisScaleMode;
using rviz_2d_plot_plugin::HorizontalAlignment;
using rviz_2d_plot_plugin::LegendPosition;
using rviz_2d_plot_plugin::Plot2DConfig;
using rviz_2d_plot_plugin::PlotControllerStatus;
using rviz_2d_plot_plugin::PlotMode;
using rviz_2d_plot_plugin::Plot2DDisplay;
using rviz_2d_plot_plugin::Plot2DDisplayTestAccessor;
using rviz_2d_plot_plugin::QoSDurability;
using rviz_2d_plot_plugin::QoSReliability;
using rviz_2d_plot_plugin::TimeSource;
using rviz_2d_plot_plugin::TopicTypeMap;
using rviz_2d_plot_plugin::VerticalAlignment;
using rviz_2d_plot_plugin::XAxisMode;
using rviz_2d_plot_plugin::XYAxisScaleMode;
using rviz_2d_plot_plugin::XYHistoryMode;
using rviz_2d_plot_plugin::test::RecordingOverlayBackend;
using rviz_2d_plot_plugin::test::childNames;
using rviz_2d_plot_plugin::test::completionsFor;
using rviz_2d_plot_plugin::test::ensureQtApplication;
using rviz_2d_plot_plugin::test::findChild;
using rviz_2d_plot_plugin::test::functionBody;
using rviz_2d_plot_plugin::test::plotDisplaySource;
using rviz_2d_plot_plugin::test::processQtEvents;
using rviz_2d_plot_plugin::test::serializeMessage;

}  // namespace

TEST(Plot2DDisplay, CreatesMvpPropertyLayout)
{
  ensureQtApplication();
  Plot2DDisplay display;

  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::pausePlot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::clearHistory(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::seriesRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::timeRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::yAxisRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::referencesRoot(display));
  ASSERT_NE(nullptr, Plot2DDisplayTestAccessor::layoutRoot(display));
  EXPECT_EQ(findChild(&display, "Pause Plot"), Plot2DDisplayTestAccessor::pausePlot(display));
  EXPECT_EQ(findChild(&display, "Clear History"), Plot2DDisplayTestAccessor::clearHistory(display));
  EXPECT_NE(nullptr, findChild(&display, "Plot Mode"));

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  EXPECT_NE(nullptr, findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count"));
  EXPECT_TRUE(series->getValue().canConvert<bool>());
  EXPECT_EQ(nullptr, findChild(series, "Enabled"));
  EXPECT_EQ(nullptr, findChild(series, "Action"));
  EXPECT_NE(nullptr, findChild(series, "Duplicate"));
  EXPECT_NE(nullptr, findChild(series, "Delete"));
  EXPECT_NE(nullptr, findChild(series, "Topic"));
  EXPECT_NE(nullptr, findChild(series, "X Field"));
  EXPECT_NE(nullptr, findChild(series, "Y Field"));
  EXPECT_NE(nullptr, findChild(series, "Field"));
  EXPECT_FALSE(findChild(series, "Field")->getHidden());
  EXPECT_TRUE(findChild(series, "X Field")->getHidden());
  EXPECT_TRUE(findChild(series, "Y Field")->getHidden());
  EXPECT_NE(nullptr, findChild(series, "Label"));
  EXPECT_NE(nullptr, findChild(series, "Unit"));
  EXPECT_NE(nullptr, findChild(series, "Color"));
  EXPECT_NE(nullptr, findChild(series, "Line Width"));
  EXPECT_NE(nullptr, findChild(series, "Line Alpha"));
  EXPECT_NE(nullptr, findChild(series, "Line Style"));
  EXPECT_NE(nullptr, findChild(series, "Plot Style"));
  EXPECT_NE(nullptr, findChild(series, "Value Scale"));
  EXPECT_NE(nullptr, findChild(series, "Value Offset"));

  auto * time = Plot2DDisplayTestAccessor::timeRoot(display);
  EXPECT_NE(nullptr, findChild(time, "Time Source"));
  EXPECT_NE(nullptr, findChild(time, "Window Seconds"));
  EXPECT_NE(nullptr, findChild(time, "XY History Mode"));
  EXPECT_TRUE(findChild(time, "XY History Mode")->getHidden());
  EXPECT_NE(nullptr, findChild(time, "Refresh Rate"));

  auto * qos = findChild(&display, "QoS");
  ASSERT_NE(nullptr, qos);
  EXPECT_NE(nullptr, findChild(qos, "Reliability"));
  EXPECT_NE(nullptr, findChild(qos, "Durability"));
  EXPECT_NE(nullptr, findChild(qos, "Depth"));

  auto * x_axis = Plot2DDisplayTestAccessor::xAxisRoot(display);
  EXPECT_TRUE(x_axis->getHidden());
  EXPECT_EQ(nullptr, findChild(x_axis, "Mode"));
  EXPECT_NE(nullptr, findChild(x_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Min"));
  EXPECT_NE(nullptr, findChild(x_axis, "X Max"));
  EXPECT_NE(nullptr, findChild(x_axis, "Axis Scale"));

  auto * y_axis = Plot2DDisplayTestAccessor::yAxisRoot(display);
  EXPECT_NE(nullptr, findChild(y_axis, "Auto Scale"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Min"));
  EXPECT_NE(nullptr, findChild(y_axis, "Y Max"));

  auto * grid = Plot2DDisplayTestAccessor::gridRoot(display);
  ASSERT_NE(nullptr, grid);
  EXPECT_NE(nullptr, findChild(grid, "Major Grid"));
  EXPECT_NE(nullptr, findChild(grid, "Minor Grid"));
  EXPECT_NE(nullptr, findChild(grid, "X Major Ticks"));
  EXPECT_NE(nullptr, findChild(grid, "Y Major Ticks"));
  EXPECT_NE(nullptr, findChild(grid, "Minor Divisions"));

  auto * references = Plot2DDisplayTestAccessor::referencesRoot(display);
  EXPECT_NE(nullptr, findChild(references, "Preset"));
  EXPECT_NE(nullptr, findChild(references, "Preset Value"));
  EXPECT_NE(nullptr, findChild(references, "Preset Tolerance"));
  EXPECT_NE(nullptr, findChild(references, "Apply Preset"));
  EXPECT_NE(nullptr, findChild(references, "Reference Count"));

  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  EXPECT_NE(nullptr, findChild(legend, "Enabled"));
  EXPECT_NE(nullptr, findChild(legend, "Show Values"));
  EXPECT_NE(nullptr, findChild(legend, "Field Name Only"));
  EXPECT_NE(nullptr, findChild(legend, "Position"));
  EXPECT_NE(nullptr, findChild(legend, "X Offset"));
  EXPECT_NE(nullptr, findChild(legend, "Y Offset"));

  auto * layout = Plot2DDisplayTestAccessor::layoutRoot(display);
  EXPECT_NE(nullptr, findChild(layout, "Width"));
  EXPECT_NE(nullptr, findChild(layout, "Height"));
  EXPECT_NE(nullptr, findChild(layout, "X Offset"));
  EXPECT_NE(nullptr, findChild(layout, "Y Offset"));
  EXPECT_NE(nullptr, findChild(layout, "Horizontal Alignment"));
  EXPECT_NE(nullptr, findChild(layout, "Vertical Alignment"));

  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  EXPECT_NE(nullptr, findChild(style, "Background Color"));
  EXPECT_NE(nullptr, findChild(style, "Background Alpha"));
  EXPECT_NE(nullptr, findChild(style, "Axis Color"));
  EXPECT_NE(nullptr, findChild(style, "Grid Color"));
  EXPECT_NE(nullptr, findChild(style, "Text Color"));
  EXPECT_NE(nullptr, findChild(style, "Font Size"));
}

TEST(Plot2DDisplay, PlotModeSwitchesBetweenTimeAndXYSeriesFields)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  auto * field = findChild(series, "Field");
  auto * x_field = findChild(series, "X Field");
  auto * y_field = findChild(series, "Y Field");
  ASSERT_NE(nullptr, field);
  ASSERT_NE(nullptr, x_field);
  ASSERT_NE(nullptr, y_field);
  auto * xy_history_mode =
    findChild(Plot2DDisplayTestAccessor::timeRoot(display), "XY History Mode");
  ASSERT_NE(nullptr, xy_history_mode);

  EXPECT_FALSE(field->getHidden());
  EXPECT_TRUE(x_field->getHidden());
  EXPECT_TRUE(y_field->getHidden());
  EXPECT_TRUE(Plot2DDisplayTestAccessor::xAxisRoot(display)->getHidden());
  EXPECT_TRUE(xy_history_mode->getHidden());

  findChild(&display, "Plot Mode")->setValue("XY");

  EXPECT_TRUE(field->getHidden());
  EXPECT_FALSE(x_field->getHidden());
  EXPECT_FALSE(y_field->getHidden());
  EXPECT_FALSE(Plot2DDisplayTestAccessor::xAxisRoot(display)->getHidden());
  EXPECT_FALSE(xy_history_mode->getHidden());
}

TEST(Plot2DDisplay, SeriesRootShowsConfiguredSourceForPlotMode)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  EXPECT_EQ(series->getName(), "Series 1");
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "Series 1");
  EXPECT_TRUE(series->getValue().toBool());

  findChild(series, "Topic")->setValue("/cmd_vel");
  findChild(series, "Field")->setValue("linear/x");

  EXPECT_EQ(series->getName(), "Series 1");
  EXPECT_TRUE(series->getValue().toBool());
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "/cmd_vel/linear/x");

  findChild(&display, "Plot Mode")->setValue("XY");

  EXPECT_TRUE(series->getValue().toBool());
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "/cmd_vel");

  findChild(series, "Label")->setValue("Linear X");

  EXPECT_TRUE(series->getValue().toBool());
  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "Linear X");
}

TEST(Plot2DDisplay, NewSeriesRowsUseSourceFallbackUntilLabelIsSet)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_count =
    findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(2);
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  ASSERT_NE(nullptr, series);

  findChild(series, "Topic")->setValue("/cmd_vel");
  findChild(series, "Field")->setValue("angular/z");

  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "/cmd_vel/angular/z");

  findChild(series, "Label")->setValue("Angular Z");

  EXPECT_EQ(series->getViewData(0, Qt::DisplayRole).toString(), "Angular Z");
}

TEST(Plot2DDisplay, PlacesActionsBeforeConfigurationGroups)
{
  ensureQtApplication();
  Plot2DDisplay display;
  const auto names = childNames(&display);

  const auto pause = std::find(names.begin(), names.end(), "Pause Plot");
  const auto clear = std::find(names.begin(), names.end(), "Clear History");
  const auto series = std::find(names.begin(), names.end(), "Series");

  ASSERT_NE(names.end(), pause);
  ASSERT_NE(names.end(), clear);
  ASSERT_NE(names.end(), series);
  EXPECT_LT(std::distance(names.begin(), pause), std::distance(names.begin(), series));
  EXPECT_LT(std::distance(names.begin(), clear), std::distance(names.begin(), series));
}

TEST(Plot2DDisplay, BooleanPropertiesUseCheckboxEditing)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  std::vector<rviz_common::properties::Property *> bool_properties{
    Plot2DDisplayTestAccessor::pausePlot(display),
    Plot2DDisplayTestAccessor::clearHistory(display),
    series,
    findChild(series, "Duplicate"),
    findChild(series, "Delete"),
    findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Auto Scale"),
    findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale"),
    findChild(Plot2DDisplayTestAccessor::gridRoot(display), "Major Grid"),
    findChild(Plot2DDisplayTestAccessor::gridRoot(display), "Minor Grid"),
  };

  for (auto * property : bool_properties) {
    ASSERT_NE(nullptr, property);
    EXPECT_TRUE(property->getValue().canConvert<bool>());
    EXPECT_FALSE(property->getViewData(1, Qt::DisplayRole).isValid());
    EXPECT_TRUE(property->getViewData(1, Qt::CheckStateRole).isValid());
    EXPECT_TRUE(property->getViewFlags(1) & Qt::ItemIsUserCheckable);
  }
}

TEST(Plot2DDisplay, BuildsPlotConfigFromProperties)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);

  series->setValue(false);
  findChild(series, "Topic")->setValue("/odom");
  findChild(&display, "Plot Mode")->setValue("XY");
  findChild(series, "X Field")->setValue("pose/pose/position/x");
  findChild(series, "Y Field")->setValue("pose/pose/position/y");
  findChild(series, "Field")->setValue("pose/pose/position/y");
  findChild(series, "Label")->setValue("Odom Position");
  findChild(series, "Unit")->setValue("m");
  findChild(series, "Color")->setValue(QColor(255, 80, 20));
  findChild(series, "Line Width")->setValue(0.5);
  findChild(series, "Line Alpha")->setValue(0.45);
  findChild(series, "Line Style")->setValue("Dash");
  findChild(series, "Plot Style")->setValue("Step");
  findChild(series, "Value Scale")->setValue(2.5);
  findChild(series, "Value Offset")->setValue(-0.75);
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Window Seconds")->setValue(45.0);
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Refresh Rate")->setValue(12.0);
  auto * time_source = findChild(Plot2DDisplayTestAccessor::timeRoot(display), "Time Source");
  ASSERT_NE(nullptr, time_source);
  time_source->setValue("Message Header Stamp");
  findChild(Plot2DDisplayTestAccessor::timeRoot(display), "XY History Mode")->setValue(
    "All Samples");
  auto * qos = findChild(&display, "QoS");
  ASSERT_NE(nullptr, qos);
  findChild(qos, "Reliability")->setValue("Best Effort");
  findChild(qos, "Durability")->setValue("Transient Local");
  findChild(qos, "Depth")->setValue(42);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Min")->setValue(-4.0);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "X Max")->setValue(4.0);
  findChild(Plot2DDisplayTestAccessor::xAxisRoot(display), "Axis Scale")->setValue("1:1");
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Auto Scale")->setValue(false);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Min")->setValue(-2.0);
  findChild(Plot2DDisplayTestAccessor::yAxisRoot(display), "Y Max")->setValue(2.0);
  findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference Count")->setValue(1);
  auto * reference =
    findChild(Plot2DDisplayTestAccessor::referencesRoot(display), "Reference 1");
  ASSERT_NE(nullptr, reference);
  reference->setValue(true);
  findChild(reference, "Y Value")->setValue(0.5);
  findChild(reference, "Tolerance")->setValue(0.2);
  findChild(reference, "Label")->setValue("Limit");
  findChild(reference, "Color")->setValue(QColor(255, 180, 60));
  findChild(reference, "Alpha")->setValue(0.6);
  findChild(reference, "Line Width")->setValue(0.25);
  findChild(reference, "Line Style")->setValue("Dot");
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Width")->setValue(420);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Height")->setValue(180);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "X Offset")->setValue(20);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Y Offset")->setValue(30);
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Horizontal Alignment")->setValue(
    "Center");
  findChild(Plot2DDisplayTestAccessor::layoutRoot(display), "Vertical Alignment")->setValue(
    "Bottom");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 1U);
  EXPECT_FALSE(config.series[0].enabled);
  EXPECT_EQ(config.series[0].topic, "/odom");
  EXPECT_EQ(config.series[0].x_field, "pose/pose/position/x");
  EXPECT_EQ(config.series[0].y_field, "pose/pose/position/y");
  EXPECT_EQ(config.series[0].field, "pose/pose/position/y");
  EXPECT_EQ(config.series[0].label, "Odom Position");
  EXPECT_EQ(config.series[0].unit, "m");
  EXPECT_EQ(config.series[0].color.red, 255);
  EXPECT_EQ(config.series[0].color.green, 80);
  EXPECT_EQ(config.series[0].color.blue, 20);
  EXPECT_DOUBLE_EQ(config.series[0].line_width, 0.5);
  EXPECT_NEAR(config.series[0].line_alpha, 0.45, 1e-6);
  EXPECT_EQ(config.series[0].line_style, rviz_2d_plot_plugin::LineStyle::Dash);
  EXPECT_EQ(config.series[0].plot_style, rviz_2d_plot_plugin::PlotStyle::Step);
  EXPECT_DOUBLE_EQ(config.series[0].value_scale, 2.5);
  EXPECT_DOUBLE_EQ(config.series[0].value_offset, -0.75);
  EXPECT_EQ(config.time.window_seconds, 45.0);
  EXPECT_EQ(config.time.refresh_rate_hz, 12.0);
  EXPECT_EQ(config.time.source, TimeSource::HeaderStamp);
  EXPECT_EQ(config.time.xy_history_mode, XYHistoryMode::AllSamples);
  EXPECT_EQ(config.qos.reliability, QoSReliability::BestEffort);
  EXPECT_EQ(config.qos.durability, QoSDurability::TransientLocal);
  EXPECT_EQ(config.qos.depth, 42);
  EXPECT_EQ(config.plot_mode, PlotMode::XY);
  EXPECT_EQ(config.x_axis.mode, XAxisMode::Field);
  EXPECT_EQ(config.x_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.x_axis.axis_scale_mode, XYAxisScaleMode::Equal);
  EXPECT_EQ(config.x_axis.fixed_min, -4.0);
  EXPECT_EQ(config.x_axis.fixed_max, 4.0);
  EXPECT_EQ(config.y_axis.scale_mode, AxisScaleMode::Fixed);
  EXPECT_EQ(config.y_axis.fixed_min, -2.0);
  EXPECT_EQ(config.y_axis.fixed_max, 2.0);
  ASSERT_EQ(config.references.size(), 1U);
  EXPECT_TRUE(config.references[0].enabled);
  EXPECT_DOUBLE_EQ(config.references[0].value, 0.5);
  EXPECT_NEAR(config.references[0].tolerance, 0.2, 1e-6);
  EXPECT_EQ(config.references[0].label, "Limit");
  EXPECT_EQ(config.references[0].color.red, 255);
  EXPECT_EQ(config.references[0].color.green, 180);
  EXPECT_EQ(config.references[0].color.blue, 60);
  EXPECT_NEAR(config.references[0].alpha, 0.6, 1e-6);
  EXPECT_DOUBLE_EQ(config.references[0].line_width, 0.25);
  EXPECT_EQ(config.references[0].line_style, rviz_2d_plot_plugin::LineStyle::Dot);
  EXPECT_EQ(config.layout.width, 420);
  EXPECT_EQ(config.layout.height, 180);
  EXPECT_EQ(config.layout.x_offset, 20);
  EXPECT_EQ(config.layout.y_offset, 30);
  EXPECT_EQ(config.layout.horizontal_alignment, HorizontalAlignment::Center);
  EXPECT_EQ(config.layout.vertical_alignment, VerticalAlignment::Bottom);
}

TEST(Plot2DDisplay, ReferencePresetAppendsNewReferences)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(1);
  auto * reference_1 = findChild(references_root, "Reference 1");
  ASSERT_NE(nullptr, reference_1);
  findChild(reference_1, "Y Value")->setValue(0.25);
  findChild(reference_1, "Label")->setValue("Existing");
  auto * preset = findChild(references_root, "Preset");
  auto * preset_value = findChild(references_root, "Preset Value");
  auto * preset_tolerance = findChild(references_root, "Preset Tolerance");
  auto * apply_preset = findChild(references_root, "Apply Preset");
  ASSERT_NE(nullptr, preset);
  ASSERT_NE(nullptr, preset_value);
  ASSERT_NE(nullptr, preset_tolerance);
  ASSERT_NE(nullptr, apply_preset);
  preset->setValue("Tolerance Band");
  preset_value->setValue(1.0);
  preset_tolerance->setValue(0.25);

  EXPECT_EQ(reference_count->getValue().toInt(), 1);
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 2"));

  apply_preset->setValue(true);

  EXPECT_FALSE(apply_preset->getValue().toBool());
  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  EXPECT_DOUBLE_EQ(findChild(reference_1, "Y Value")->getValue().toDouble(), 0.25);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "Existing");
  EXPECT_DOUBLE_EQ(findChild(reference_2, "Y Value")->getValue().toDouble(), 1.0);
  EXPECT_NEAR(findChild(reference_2, "Tolerance")->getValue().toDouble(), 0.25, 1e-6);
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "Target");
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 3"));
  EXPECT_EQ(preset->getValue().toString(), "Tolerance Band");

  preset->setValue("Unknown Preset");
  apply_preset->setValue(true);

  EXPECT_FALSE(apply_preset->getValue().toBool());
  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 3"));
}

TEST(Plot2DDisplay, ReferenceRowsUseRootCheckboxAndLabelSummary)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(1);
  auto * reference = findChild(references_root, "Reference 1");
  ASSERT_NE(nullptr, reference);

  EXPECT_TRUE(reference->getValue().canConvert<bool>());
  EXPECT_TRUE(reference->getValue().toBool());
  EXPECT_FALSE(reference->getViewData(1, Qt::DisplayRole).isValid());
  EXPECT_TRUE(reference->getViewData(1, Qt::CheckStateRole).isValid());
  EXPECT_TRUE(reference->getViewFlags(1) & Qt::ItemIsUserCheckable);
  EXPECT_EQ(reference->getViewData(0, Qt::DisplayRole).toString(), "Reference 1");
  EXPECT_EQ(nullptr, findChild(reference, "Enabled"));
  EXPECT_EQ(nullptr, findChild(reference, "Action"));

  findChild(reference, "Label")->setValue("CTE = 0");

  EXPECT_EQ(reference->getViewData(0, Qt::DisplayRole).toString(), "CTE = 0");

  reference->setValue(false);
  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);
  ASSERT_EQ(config.references.size(), 1U);
  EXPECT_FALSE(config.references[0].enabled);
}

TEST(Plot2DDisplay, MapsLegendPropertiesToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * legend = Plot2DDisplayTestAccessor::legendRoot(display);
  ASSERT_NE(nullptr, legend);
  findChild(legend, "Enabled")->setValue(false);
  findChild(legend, "Show Values")->setValue(false);
  findChild(legend, "Field Name Only")->setValue(true);
  findChild(legend, "Position")->setValue("Bottom Right");
  findChild(legend, "X Offset")->setValue(12);
  findChild(legend, "Y Offset")->setValue(8);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_FALSE(settings.show_legend);
  EXPECT_FALSE(settings.show_latest_values);
  EXPECT_TRUE(settings.legend_field_name_only);
  EXPECT_EQ(settings.legend_position, LegendPosition::BottomRight);
  EXPECT_EQ(settings.legend_x_offset, 12);
  EXPECT_EQ(settings.legend_y_offset, 8);
}

TEST(Plot2DDisplay, MapsGridPropertiesToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * grid = Plot2DDisplayTestAccessor::gridRoot(display);
  ASSERT_NE(nullptr, grid);
  findChild(grid, "Major Grid")->setValue(false);
  findChild(grid, "Minor Grid")->setValue(true);
  findChild(grid, "X Major Ticks")->setValue(4);
  findChild(grid, "Y Major Ticks")->setValue(7);
  findChild(grid, "Minor Divisions")->setValue(2);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_FALSE(settings.show_major_grid);
  EXPECT_TRUE(settings.show_minor_grid);
  EXPECT_EQ(settings.x_major_tick_count, 4);
  EXPECT_EQ(settings.y_major_tick_count, 7);
  EXPECT_EQ(settings.minor_grid_divisions, 2);
}

TEST(Plot2DDisplay, MapsBackgroundAlphaToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  findChild(style, "Background Color")->setValue(QColor(10, 20, 30));
  findChild(style, "Background Alpha")->setValue(0.25);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.background_color.red(), 10);
  EXPECT_EQ(settings.background_color.green(), 20);
  EXPECT_EQ(settings.background_color.blue(), 30);
  EXPECT_NEAR(settings.background_color.alphaF(), 0.25, 1e-3);
}

TEST(Plot2DDisplay, MapsFontSizeToRenderSettings)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * style = findChild(&display, "Style");
  ASSERT_NE(nullptr, style);
  findChild(style, "Font Size")->setValue(12);

  const auto settings = Plot2DDisplayTestAccessor::renderSettingsFromProperties(display);
  EXPECT_EQ(settings.font_size, 12);
}

TEST(Plot2DDisplay, AssignsDistinctDefaultColorsToNewSeries)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_count =
    findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(3);
  auto * series_1 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  auto * series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  auto * series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);

  auto * color_1 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_1, "Color"));
  auto * color_2 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_2, "Color"));
  auto * color_3 =
    dynamic_cast<rviz_common::properties::ColorProperty *>(findChild(series_3, "Color"));
  ASSERT_NE(nullptr, color_1);
  ASSERT_NE(nullptr, color_2);
  ASSERT_NE(nullptr, color_3);

  EXPECT_NE(color_1->getColor(), color_2->getColor());
  EXPECT_NE(color_2->getColor(), color_3->getColor());
}

TEST(Plot2DDisplay, BuildsPlotConfigFromMultipleSeriesProperties)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_count =
    findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(2);
  auto * series_1 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  auto * series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);

  findChild(series_1, "Topic")->setValue("/cmd_vel");
  findChild(series_1, "Field")->setValue("linear/x");
  findChild(series_1, "Label")->setValue("Linear X");
  findChild(series_2, "Topic")->setValue("/cmd_vel");
  findChild(series_2, "Field")->setValue("angular/z");
  findChild(series_2, "Label")->setValue("Angular Z");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);

  ASSERT_EQ(config.series.size(), 2U);
  EXPECT_EQ(config.series[0].topic, "/cmd_vel");
  EXPECT_EQ(config.series[0].field, "linear/x");
  EXPECT_EQ(config.series[0].label, "Linear X");
  EXPECT_EQ(config.series[1].topic, "/cmd_vel");
  EXPECT_EQ(config.series[1].field, "angular/z");
  EXPECT_EQ(config.series[1].label, "Angular Z");
}

TEST(Plot2DDisplay, LoadsDynamicSeriesAndReferenceCountsBeforeChildren)
{
  ensureQtApplication();
  Plot2DDisplay display;

  rviz_common::Config config;
  auto series = config.mapMakeChild("Series");
  series.mapSetValue("Series Count", 2);
  auto series_1 = series.mapMakeChild("Series 1");
  series_1.mapSetValue("Topic", "/cmd_vel");
  series_1.mapSetValue("Field", "linear/x");
  series_1.mapSetValue("Label", "Linear X");
  auto series_2 = series.mapMakeChild("Series 2");
  series_2.mapSetValue("Topic", "/cmd_vel");
  series_2.mapSetValue("Field", "angular/z");
  series_2.mapSetValue("Label", "Angular Z");

  auto references = config.mapMakeChild("References");
  references.mapSetValue("Reference Count", 1);
  auto reference_1 = references.mapMakeChild("Reference 1");
  reference_1.mapSetValue("Y Value", 3.0);
  reference_1.mapSetValue("Label", "Upper Limit");

  testing::internal::CaptureStdout();
  display.load(config);
  const std::string load_output = testing::internal::GetCapturedStdout();

  const Plot2DConfig loaded = Plot2DDisplayTestAccessor::configFromProperties(display);
  ASSERT_EQ(loaded.series.size(), 2U);
  EXPECT_EQ(loaded.series[0].topic, "/cmd_vel");
  EXPECT_EQ(loaded.series[0].field, "linear/x");
  EXPECT_EQ(loaded.series[0].label, "Linear X");
  EXPECT_EQ(loaded.series[1].topic, "/cmd_vel");
  EXPECT_EQ(loaded.series[1].field, "angular/z");
  EXPECT_EQ(loaded.series[1].label, "Angular Z");
  ASSERT_EQ(loaded.references.size(), 1U);
  EXPECT_DOUBLE_EQ(loaded.references[0].value, 3.0);
  EXPECT_EQ(loaded.references[0].label, "Upper Limit");
  EXPECT_EQ(load_output.find("unexpected QVariant type"), std::string::npos);
}
