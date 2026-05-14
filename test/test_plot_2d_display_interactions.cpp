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

TEST(Plot2DDisplay, ReferenceCommandCheckboxesDuplicateAndDeleteReference)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(2);
  auto * reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  findChild(reference_1, "Y Value")->setValue(0.25);
  findChild(reference_1, "Label")->setValue("First");
  findChild(reference_2, "Y Value")->setValue(0.75);
  findChild(reference_2, "Tolerance")->setValue(0.1);
  findChild(reference_2, "Label")->setValue("Second");

  auto * duplicate = findChild(reference_1, "Duplicate");
  ASSERT_NE(nullptr, duplicate);
  EXPECT_FALSE(duplicate->shouldBeSaved());
  duplicate->setValue(true);
  processQtEvents();

  EXPECT_EQ(reference_count->getValue().toInt(), 3);
  reference_1 = findChild(references_root, "Reference 1");
  reference_2 = findChild(references_root, "Reference 2");
  auto * reference_3 = findChild(references_root, "Reference 3");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  ASSERT_NE(nullptr, reference_3);
  EXPECT_DOUBLE_EQ(findChild(reference_2, "Y Value")->getValue().toDouble(), 0.25);
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "First Copy");
  EXPECT_DOUBLE_EQ(findChild(reference_3, "Y Value")->getValue().toDouble(), 0.75);

  auto * delete_reference = findChild(reference_2, "Delete");
  ASSERT_NE(nullptr, delete_reference);
  EXPECT_FALSE(delete_reference->shouldBeSaved());
  delete_reference->setValue(true);
  processQtEvents();

  EXPECT_EQ(reference_count->getValue().toInt(), 2);
  reference_1 = findChild(references_root, "Reference 1");
  reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  EXPECT_DOUBLE_EQ(findChild(reference_1, "Y Value")->getValue().toDouble(), 0.25);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "First");
  EXPECT_DOUBLE_EQ(findChild(reference_2, "Y Value")->getValue().toDouble(), 0.75);
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "Second");
  EXPECT_EQ(nullptr, findChild(references_root, "Reference 3"));
}

TEST(Plot2DDisplay, ReferenceActionsPreserveRowExpansionState)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(2);
  auto * reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  findChild(reference_1, "Label")->setValue("First");
  findChild(reference_2, "Label")->setValue("Second");
  reference_1->expand();
  reference_2->collapse();
  ASSERT_TRUE(reference_1->isExpanded());
  ASSERT_FALSE(reference_2->isExpanded());

  findChild(reference_1, "Duplicate")->setValue(true);
  processQtEvents();

  reference_1 = findChild(references_root, "Reference 1");
  reference_2 = findChild(references_root, "Reference 2");
  auto * reference_3 = findChild(references_root, "Reference 3");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  ASSERT_NE(nullptr, reference_3);
  EXPECT_TRUE(reference_1->isExpanded());
  EXPECT_TRUE(reference_2->isExpanded());
  EXPECT_FALSE(reference_3->isExpanded());

  findChild(reference_2, "Delete")->setValue(true);
  processQtEvents();

  reference_1 = findChild(references_root, "Reference 1");
  reference_2 = findChild(references_root, "Reference 2");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  EXPECT_TRUE(reference_1->isExpanded());
  EXPECT_FALSE(reference_2->isExpanded());
}

TEST(Plot2DDisplay, ReferenceRowsUseNativeDragDropReordering)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * references_root = Plot2DDisplayTestAccessor::referencesRoot(display);
  auto * reference_count = findChild(references_root, "Reference Count");
  ASSERT_NE(nullptr, reference_count);

  reference_count->setValue(3);
  auto * reference_1 = findChild(references_root, "Reference 1");
  auto * reference_2 = findChild(references_root, "Reference 2");
  auto * reference_3 = findChild(references_root, "Reference 3");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  ASSERT_NE(nullptr, reference_3);
  EXPECT_TRUE(references_root->getViewFlags(0) & Qt::ItemIsDropEnabled);
  EXPECT_TRUE(reference_1->getViewFlags(0) & Qt::ItemIsDragEnabled);

  findChild(reference_1, "Label")->setValue("First");
  findChild(reference_1, "Y Value")->setValue(1.0);
  findChild(reference_2, "Label")->setValue("Second");
  findChild(reference_2, "Y Value")->setValue(2.0);
  findChild(reference_3, "Label")->setValue("Third");
  findChild(reference_3, "Y Value")->setValue(3.0);

  rviz_common::properties::Property * moved = references_root->takeChildAt(7);
  ASSERT_NE(nullptr, moved);
  references_root->addChild(moved, 6);
  processQtEvents();

  reference_1 = findChild(references_root, "Reference 1");
  reference_2 = findChild(references_root, "Reference 2");
  reference_3 = findChild(references_root, "Reference 3");
  ASSERT_NE(nullptr, reference_1);
  ASSERT_NE(nullptr, reference_2);
  ASSERT_NE(nullptr, reference_3);
  EXPECT_EQ(findChild(reference_1, "Label")->getValue().toString(), "First");
  EXPECT_EQ(findChild(reference_2, "Label")->getValue().toString(), "Third");
  EXPECT_EQ(findChild(reference_3, "Label")->getValue().toString(), "Second");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);
  ASSERT_EQ(config.references.size(), 3U);
  EXPECT_EQ(config.references[0].label, "First");
  EXPECT_EQ(config.references[1].label, "Third");
  EXPECT_EQ(config.references[2].label, "Second");
}

TEST(Plot2DDisplay, SeriesCommandCheckboxesDuplicateAndDeleteSeries)
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
  findChild(series_1, "Label")->setValue("First");
  findChild(series_1, "Topic")->setValue("/first");
  findChild(series_1, "Field")->setValue("data");
  findChild(series_2, "Label")->setValue("Second");

  ASSERT_EQ(nullptr, findChild(series_1, "Action"));
  auto * duplicate = findChild(series_1, "Duplicate");
  ASSERT_NE(nullptr, duplicate);
  EXPECT_FALSE(duplicate->shouldBeSaved());
  duplicate->setValue(true);
  processQtEvents();

  EXPECT_EQ(series_count->getValue().toInt(), 3);
  series_2 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 2");
  auto * series_3 = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3");
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_EQ(findChild(series_2, "Label")->getValue().toString(), "First Copy");
  EXPECT_EQ(findChild(series_2, "Topic")->getValue().toString(), "/first");
  EXPECT_EQ(findChild(series_3, "Label")->getValue().toString(), "Second");

  auto * delete_series = findChild(series_3, "Delete");
  ASSERT_NE(nullptr, delete_series);
  EXPECT_FALSE(delete_series->shouldBeSaved());
  delete_series->setValue(true);
  processQtEvents();

  EXPECT_EQ(series_count->getValue().toInt(), 2);
  EXPECT_EQ(nullptr, findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 3"));
}

TEST(Plot2DDisplay, SeriesActionsPreserveRowExpansionState)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_root = Plot2DDisplayTestAccessor::seriesRoot(display);
  auto * series_count = findChild(series_root, "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(2);
  auto * series_1 = findChild(series_root, "Series 1");
  auto * series_2 = findChild(series_root, "Series 2");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  findChild(series_1, "Label")->setValue("First");
  findChild(series_2, "Label")->setValue("Second");
  series_1->expand();
  series_2->collapse();
  ASSERT_TRUE(series_1->isExpanded());
  ASSERT_FALSE(series_2->isExpanded());

  findChild(series_1, "Duplicate")->setValue(true);
  processQtEvents();

  series_1 = findChild(series_root, "Series 1");
  series_2 = findChild(series_root, "Series 2");
  auto * series_3 = findChild(series_root, "Series 3");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_TRUE(series_1->isExpanded());
  EXPECT_TRUE(series_2->isExpanded());
  EXPECT_FALSE(series_3->isExpanded());

  findChild(series_2, "Delete")->setValue(true);
  processQtEvents();

  series_1 = findChild(series_root, "Series 1");
  series_2 = findChild(series_root, "Series 2");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  EXPECT_TRUE(series_1->isExpanded());
  EXPECT_FALSE(series_2->isExpanded());
}

TEST(Plot2DDisplay, SeriesRowsUseNativeDragDropReordering)
{
  ensureQtApplication();
  Plot2DDisplay display;
  auto * series_root = Plot2DDisplayTestAccessor::seriesRoot(display);
  auto * series_count = findChild(series_root, "Series Count");
  ASSERT_NE(nullptr, series_count);

  series_count->setValue(3);
  auto * series_1 = findChild(series_root, "Series 1");
  auto * series_2 = findChild(series_root, "Series 2");
  auto * series_3 = findChild(series_root, "Series 3");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_TRUE(series_root->getViewFlags(0) & Qt::ItemIsDropEnabled);
  EXPECT_TRUE(series_1->getViewFlags(0) & Qt::ItemIsDragEnabled);
  EXPECT_EQ(nullptr, findChild(series_1, "Move Up"));
  EXPECT_EQ(nullptr, findChild(series_1, "Move Down"));

  findChild(series_1, "Label")->setValue("First");
  findChild(series_1, "Topic")->setValue("/first");
  findChild(series_2, "Label")->setValue("Second");
  findChild(series_2, "Topic")->setValue("/second");
  findChild(series_3, "Label")->setValue("Third");
  findChild(series_3, "Topic")->setValue("/third");

  rviz_common::properties::Property * moved = series_root->takeChildAt(3);
  ASSERT_NE(nullptr, moved);
  series_root->addChild(moved, 2);
  processQtEvents();

  EXPECT_EQ(series_root->childAt(0)->getName(), "Series Count");
  series_1 = findChild(series_root, "Series 1");
  series_2 = findChild(series_root, "Series 2");
  series_3 = findChild(series_root, "Series 3");
  ASSERT_NE(nullptr, series_1);
  ASSERT_NE(nullptr, series_2);
  ASSERT_NE(nullptr, series_3);
  EXPECT_EQ(findChild(series_1, "Label")->getValue().toString(), "First");
  EXPECT_EQ(findChild(series_2, "Label")->getValue().toString(), "Third");
  EXPECT_EQ(findChild(series_2, "Topic")->getValue().toString(), "/third");
  EXPECT_EQ(findChild(series_3, "Label")->getValue().toString(), "Second");

  const Plot2DConfig config = Plot2DDisplayTestAccessor::configFromProperties(display);
  ASSERT_EQ(config.series.size(), 3U);
  EXPECT_EQ(config.series[0].label, "First");
  EXPECT_EQ(config.series[1].label, "Third");
  EXPECT_EQ(config.series[2].label, "Second");
}

TEST(Plot2DDisplay, TopicAndFieldEditorsFilterOptionsByContainsWhileTyping)
{
  ensureQtApplication();
  Plot2DDisplay display;
  Plot2DDisplayTestAccessor::setTopics(
    display,
    TopicTypeMap{{"/cmd_vel_out", {"geometry_msgs/msg/Twist"}}});

  auto * series = findChild(Plot2DDisplayTestAccessor::seriesRoot(display), "Series 1");
  ASSERT_NE(nullptr, series);
  auto * topic_property =
    qobject_cast<rviz_common::properties::EditableEnumProperty *>(
    findChild(series, "Topic"));
  ASSERT_NE(nullptr, topic_property);

  QStyleOptionViewItem option;
  std::unique_ptr<QWidget> topic_editor(topic_property->createEditor(nullptr, option));
  auto * topic_combo = qobject_cast<QComboBox *>(topic_editor.get());
  ASSERT_NE(nullptr, topic_combo);
  ASSERT_NE(nullptr, topic_combo->completer());
  EXPECT_EQ(topic_combo->completer()->filterMode(), Qt::MatchContains);
  const std::vector<QString> topic_completions =
    completionsFor(topic_combo->completer(), "cmd");
  EXPECT_NE(
    std::find(topic_completions.begin(), topic_completions.end(), "/cmd_vel_out"),
    topic_completions.end());

  topic_property->setValue("/cmd_vel_out");
  auto * field_property =
    qobject_cast<rviz_common::properties::EditableEnumProperty *>(
    findChild(series, "Field"));
  ASSERT_NE(nullptr, field_property);

  std::unique_ptr<QWidget> field_editor(field_property->createEditor(nullptr, option));
  auto * field_combo = qobject_cast<QComboBox *>(field_editor.get());
  ASSERT_NE(nullptr, field_combo);
  ASSERT_NE(nullptr, field_combo->completer());
  EXPECT_EQ(field_combo->completer()->filterMode(), Qt::MatchContains);
  const std::vector<QString> field_completions =
    completionsFor(field_combo->completer(), "x");
  EXPECT_NE(
    std::find(field_completions.begin(), field_completions.end(), "linear/x"),
    field_completions.end());
}
