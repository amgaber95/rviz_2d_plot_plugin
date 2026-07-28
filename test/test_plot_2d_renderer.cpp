// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QRect>

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <vector>

#include "rviz_2d_plot_plugin/plot_2d_renderer.hpp"
#include "rviz_2d_plot_plugin/sample_buffer.hpp"

using rviz_2d_plot_plugin::Plot2DRenderer;
using rviz_2d_plot_plugin::PlotRenderSettings;
using rviz_2d_plot_plugin::PlotSample;
using rviz_2d_plot_plugin::PlotStyle;
using rviz_2d_plot_plugin::RenderableReference;
using rviz_2d_plot_plugin::RenderableSeries;
using rviz_2d_plot_plugin::LineStyle;
using rviz_2d_plot_plugin::LegendPosition;
using rviz_2d_plot_plugin::SeriesAxis;
using rviz_2d_plot_plugin::XAxisMode;

namespace
{

void ensureQtApplication()
{
  if (QApplication::instance()) {
    return;
  }

  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char app_name[] = "test_plot_2d_renderer";
  static char * argv[] = {app_name, nullptr};
  static QApplication application(argc, argv);
}

bool hasDifferentPixel(const QImage & image, const QColor & color)
{
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (image.pixelColor(x, y) != color) {
        return true;
      }
    }
  }
  return false;
}

int countDifferentPixels(const QImage & lhs, const QImage & rhs)
{
  if (lhs.size() != rhs.size()) {
    return 1;
  }

  int count = 0;
  for (int y = 0; y < lhs.height(); ++y) {
    for (int x = 0; x < lhs.width(); ++x) {
      if (lhs.pixelColor(x, y) != rhs.pixelColor(x, y)) {
        ++count;
      }
    }
  }
  return count;
}

int countPixelsCloseTo(const QImage & image, const QColor & target)
{
  int count = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (std::abs(pixel.red() - target.red()) < 40 &&
        std::abs(pixel.green() - target.green()) < 40 &&
        std::abs(pixel.blue() - target.blue()) < 40)
      {
        ++count;
      }
    }
  }
  return count;
}

int countPixelsCloseToInRect(const QImage & image, const QColor & target, const QRect & rect)
{
  int count = 0;
  const QRect bounded = rect.intersected(image.rect());
  for (int y = bounded.top(); y <= bounded.bottom(); ++y) {
    for (int x = bounded.left(); x <= bounded.right(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (std::abs(pixel.red() - target.red()) < 40 &&
        std::abs(pixel.green() - target.green()) < 40 &&
        std::abs(pixel.blue() - target.blue()) < 40)
      {
        ++count;
      }
    }
  }
  return count;
}

QRect coloredPixelBounds(const QImage & image, const QColor & target)
{
  QRect bounds;
  int left = std::numeric_limits<int>::max();
  int right = std::numeric_limits<int>::min();
  int top = std::numeric_limits<int>::max();
  int bottom = std::numeric_limits<int>::min();

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (std::abs(pixel.red() - target.red()) >= 40 ||
        std::abs(pixel.green() - target.green()) >= 40 ||
        std::abs(pixel.blue() - target.blue()) >= 40)
      {
        continue;
      }
      left = std::min(left, x);
      right = std::max(right, x);
      top = std::min(top, y);
      bottom = std::max(bottom, y);
    }
  }

  if (left <= right && top <= bottom) {
    bounds = QRect(QPoint(left, top), QPoint(right, bottom));
  }
  return bounds;
}

int firstColumnDifferentFrom(const QImage & image, const QColor & background)
{
  for (int x = 0; x < image.width(); ++x) {
    for (int y = 0; y < image.height(); ++y) {
      if (image.pixelColor(x, y) != background) {
        return x;
      }
    }
  }
  return -1;
}

RenderableSeries horizontalSeries()
{
  RenderableSeries series;
  series.label = "Styled";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{5.0, 0.0}, {10.0, 0.0}};
  return series;
}

}  // namespace

TEST(Plot2DRenderer, RendersConfiguredImageSize)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;

  const QImage image = renderer.render(settings, {});

  EXPECT_FALSE(image.isNull());
  EXPECT_EQ(image.width(), settings.width);
  EXPECT_EQ(image.height(), settings.height);
}

TEST(Plot2DRenderer, RendersNonEmptyImageWithGridAndAxes)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;

  const QImage image = renderer.render(settings, {});

  EXPECT_TRUE(hasDifferentPixel(image, settings.background_color));
}

TEST(Plot2DRenderer, UsesCompactLeftGutterForShortYAxisLabels)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.show_legend = false;
  settings.show_major_grid = false;
  settings.show_minor_grid = false;
  settings.text_color = QColor(255, 255, 255, 0);

  const QImage image = renderer.render(settings, {});

  EXPECT_LT(firstColumnDifferentFrom(image, settings.background_color), 36);
}

TEST(Plot2DRenderer, KeepsWiderLeftGutterForLongYAxisLabels)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1000.0;
  settings.fixed_y_max = 1000.0;
  settings.show_legend = false;
  settings.show_major_grid = false;
  settings.show_minor_grid = false;
  settings.text_color = QColor(255, 255, 255, 0);

  const QImage image = renderer.render(settings, {});

  EXPECT_GE(firstColumnDifferentFrom(image, settings.background_color), 36);
}

TEST(Plot2DRenderer, MinorGridAddsSubtleIntermediateLines)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 4.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.grid_color = QColor(180, 180, 180, 160);
  settings.x_major_tick_count = 3;
  settings.y_major_tick_count = 3;
  settings.minor_grid_divisions = 1;

  settings.show_minor_grid = false;
  const QImage major_only = renderer.render(settings, {});
  settings.show_minor_grid = true;
  const QImage with_minor = renderer.render(settings, {});

  EXPECT_GT(countDifferentPixels(with_minor, major_only), 0);
}

TEST(Plot2DRenderer, TimeSeriesGridAndLabelsStayAnchoredAsNowAdvances)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 100.0;
  settings.window_seconds = 50.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.x_major_tick_count = 6;
  settings.minor_grid_divisions = 0;

  const QImage first = renderer.render(settings, {});
  settings.now = 101.0;
  const QImage second = renderer.render(settings, {});

  EXPECT_EQ(countDifferentPixels(first, second), 0);
}

TEST(Plot2DRenderer, DrawsSeriesSamples)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;

  std::vector<RenderableSeries> series{
    RenderableSeries{
      "Linear X",
      QColor(80, 170, 255),
      std::vector<PlotSample>{{8.0, 1.0}, {9.0, 2.0}, {10.0, 1.5}},
      ""}};

  const QImage image = renderer.render(settings, series);

  EXPECT_TRUE(hasDifferentPixel(image, settings.background_color));
  EXPECT_NE(image.pixelColor(1, 1), QColor(Qt::transparent));
}

TEST(Plot2DRenderer, UsesSampleXValuesInFieldXAxisMode)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 20.0;
  settings.window_seconds = 5.0;
  settings.x_axis_mode = XAxisMode::Field;
  settings.x_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_x_min = 0.0;
  settings.fixed_x_max = 10.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "XY";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{
    PlotSample{20.0, 2.0, 0.0},
    PlotSample{20.5, 8.0, 0.0}};

  const QImage image = renderer.render(settings, {series});

  EXPECT_GT(
    countPixelsCloseToInRect(image, QColor(250, 40, 40), QRect(90, 70, 160, 30)),
    0);
}

TEST(Plot2DRenderer, EqualXYAxisScaleUsesMatchingPixelUnits)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 360;
  settings.height = 220;
  settings.x_axis_mode = XAxisMode::Field;
  settings.x_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_x_min = 0.0;
  settings.fixed_x_max = 1.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = 0.0;
  settings.fixed_y_max = 1.0;
  settings.xy_axis_scale_mode = rviz_2d_plot_plugin::XYAxisScaleMode::Equal;
  settings.show_legend = false;

  RenderableSeries series;
  series.label = "Unit Square";
  series.color = QColor(250, 40, 40);
  series.line_width = 3.0;
  series.plot_style = PlotStyle::Step;
  series.samples = std::vector<PlotSample>{
    PlotSample{0.0, 0.0, 0.0},
    PlotSample{0.0, 1.0, 1.0}};

  const QImage image = renderer.render(settings, {series});
  const QRect bounds = coloredPixelBounds(image, series.color);

  ASSERT_FALSE(bounds.isNull());
  EXPECT_NEAR(bounds.width(), bounds.height(), 4);
}

TEST(Plot2DRenderer, IndependentXYAxisScaleKeepsSeparateAxisRanges)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 360;
  settings.height = 220;
  settings.x_axis_mode = XAxisMode::Field;
  settings.x_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_x_min = 0.0;
  settings.fixed_x_max = 1.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = 0.0;
  settings.fixed_y_max = 1.0;
  settings.xy_axis_scale_mode = rviz_2d_plot_plugin::XYAxisScaleMode::Independent;
  settings.show_legend = false;

  RenderableSeries series;
  series.label = "Unit Square";
  series.color = QColor(250, 40, 40);
  series.line_width = 3.0;
  series.plot_style = PlotStyle::Step;
  series.samples = std::vector<PlotSample>{
    PlotSample{0.0, 0.0, 0.0},
    PlotSample{0.0, 1.0, 1.0}};

  const QImage image = renderer.render(settings, {series});
  const QRect bounds = coloredPixelBounds(image, series.color);

  ASSERT_FALSE(bounds.isNull());
  EXPECT_GT(bounds.width(), bounds.height() + 80);
}

TEST(Plot2DRenderer, RightAxisSeriesUsesIndependentYScale)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 360;
  settings.height = 220;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.right_y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_right_y_min = -10.0;
  settings.fixed_right_y_max = 10.0;
  settings.show_legend = false;

  RenderableSeries left_series;
  left_series.label = "Left";
  left_series.color = QColor(250, 40, 40);
  left_series.axis = SeriesAxis::Left;
  left_series.plot_style = PlotStyle::Points;
  left_series.samples = std::vector<PlotSample>{{10.0, 0.8}};

  RenderableSeries right_series;
  right_series.label = "Right";
  right_series.color = QColor(40, 200, 255);
  right_series.axis = SeriesAxis::Right;
  right_series.plot_style = PlotStyle::Points;
  right_series.samples = std::vector<PlotSample>{{10.0, 0.8}};

  const QImage image = renderer.render(settings, {left_series, right_series});
  const QRect left_bounds = coloredPixelBounds(image, left_series.color);
  const QRect right_bounds = coloredPixelBounds(image, right_series.color);

  ASSERT_FALSE(left_bounds.isNull());
  ASSERT_FALSE(right_bounds.isNull());
  EXPECT_GT(right_bounds.center().y() - left_bounds.center().y(), 20);
}

TEST(Plot2DRenderer, AppliesConfiguredLineWidth)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries thin = horizontalSeries();
  thin.line_width = 1.0;
  RenderableSeries thick = horizontalSeries();
  thick.line_width = 7.0;

  const QImage thin_image = renderer.render(settings, {thin});
  const QImage thick_image = renderer.render(settings, {thick});

  EXPECT_GT(
    countPixelsCloseTo(thick_image, QColor(250, 40, 40)),
    countPixelsCloseTo(thin_image, QColor(250, 40, 40)) * 2);
}

TEST(Plot2DRenderer, SupportsSubpixelSeriesLineWidth)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries subpixel = horizontalSeries();
  subpixel.line_width = 0.5;
  RenderableSeries single_pixel = horizontalSeries();
  single_pixel.line_width = 1.0;

  const QImage subpixel_image = renderer.render(settings, {subpixel});
  const QImage single_pixel_image = renderer.render(settings, {single_pixel});

  EXPECT_GT(countDifferentPixels(subpixel_image, single_pixel_image), 0);
}

TEST(Plot2DRenderer, AppliesConfiguredDashLineStyle)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries solid = horizontalSeries();
  solid.line_width = 3.0;
  solid.line_style = LineStyle::Solid;
  RenderableSeries dash = horizontalSeries();
  dash.line_width = 3.0;
  dash.line_style = LineStyle::Dash;

  const QImage solid_image = renderer.render(settings, {solid});
  const QImage dash_image = renderer.render(settings, {dash});

  EXPECT_LT(
    countPixelsCloseTo(dash_image, QColor(250, 40, 40)),
    countPixelsCloseTo(solid_image, QColor(250, 40, 40)));
}

TEST(Plot2DRenderer, PointsStyleDrawsSingleSample)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "Point";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{8.0, 0.0}};
  series.plot_style = PlotStyle::Points;

  const QImage image = renderer.render(settings, {series});

  EXPECT_GT(countPixelsCloseTo(image, QColor(250, 40, 40)), 0);
}

TEST(Plot2DRenderer, StepStyleDrawsMoreOrthogonalSegmentsThanLineStyle)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries line = horizontalSeries();
  line.samples = std::vector<PlotSample>{{5.0, -0.7}, {10.0, 0.7}};
  line.line_width = 2.0;
  line.plot_style = PlotStyle::Line;
  RenderableSeries step = line;
  step.plot_style = PlotStyle::Step;

  const QImage line_image = renderer.render(settings, {line});
  const QImage step_image = renderer.render(settings, {step});

  EXPECT_GT(
    countPixelsCloseTo(step_image, QColor(250, 40, 40)),
    countPixelsCloseTo(line_image, QColor(250, 40, 40)));
}

TEST(Plot2DRenderer, DrawsEnabledReferenceLines)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableReference reference;
  reference.value = 0.0;
  reference.color = QColor(255, 180, 60);
  reference.line_width = 3.0;

  const QImage image = renderer.render(settings, {}, {reference});

  EXPECT_GT(countPixelsCloseTo(image, QColor(255, 180, 60)), 0);
}

TEST(Plot2DRenderer, SupportsSubpixelReferenceLineWidth)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableReference subpixel;
  subpixel.value = 0.0;
  subpixel.color = QColor(255, 180, 60);
  subpixel.line_width = 0.5;
  RenderableReference single_pixel = subpixel;
  single_pixel.line_width = 1.0;

  const QImage subpixel_image = renderer.render(settings, {}, {subpixel});
  const QImage single_pixel_image = renderer.render(settings, {}, {single_pixel});

  EXPECT_GT(countDifferentPixels(subpixel_image, single_pixel_image), 0);
}

TEST(Plot2DRenderer, DrawsReferenceToleranceBands)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.show_major_grid = false;
  settings.show_minor_grid = false;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableReference reference;
  reference.value = 0.0;
  reference.color = QColor(255, 180, 60);
  reference.line_width = 2.0;

  const QImage line_only = renderer.render(settings, {}, {reference});
  reference.tolerance = 0.35;
  const QImage with_band = renderer.render(settings, {}, {reference});

  EXPECT_GT(countDifferentPixels(with_band, line_only), 1000);
}

TEST(Plot2DRenderer, PlacesLegendAtConfiguredCorner)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.legend_position = LegendPosition::BottomRight;

  RenderableSeries series;
  series.label = "L";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{10.0, 0.0}};

  const QImage image = renderer.render(settings, {series});

  EXPECT_GT(
    countPixelsCloseToInRect(image, QColor(250, 40, 40), QRect(214, 118, 90, 24)),
    0);
  EXPECT_EQ(
    countPixelsCloseToInRect(image, QColor(250, 40, 40), QRect(42, 16, 130, 28)),
    0);
}

TEST(Plot2DRenderer, DrawsConfiguredLegendEntryBeforeSamplesArrive)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "Waiting";
  series.color = QColor(250, 40, 40);

  settings.show_latest_values = true;
  const QImage with_values = renderer.render(settings, {series});
  settings.show_latest_values = false;
  const QImage without_values = renderer.render(settings, {series});

  EXPECT_GT(countPixelsCloseTo(with_values, QColor(250, 40, 40)), 0);
  EXPECT_EQ(countDifferentPixels(with_values, without_values), 0);
}

TEST(Plot2DRenderer, HidesLegendWhenDisabled)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "L";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{10.0, 0.0}};

  settings.show_legend = true;
  const QImage visible = renderer.render(settings, {series});
  settings.show_legend = false;
  const QImage hidden = renderer.render(settings, {series});

  EXPECT_GT(countPixelsCloseTo(visible, QColor(250, 40, 40)), 0);
  EXPECT_EQ(countPixelsCloseTo(hidden, QColor(250, 40, 40)), 0);
}

TEST(Plot2DRenderer, OmitsLatestValuesFromLegendWhenConfigured)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "Speed";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{10.0, 0.0}};

  settings.show_latest_values = true;
  const QImage with_values = renderer.render(settings, {series});
  settings.show_latest_values = false;
  const QImage without_values = renderer.render(settings, {series});

  EXPECT_GT(countDifferentPixels(with_values, without_values), 0);
}

TEST(Plot2DRenderer, RightLegendLatestValuesAreIndependent)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.right_y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_right_y_min = -1.0;
  settings.fixed_right_y_max = 1.0;
  settings.show_latest_values = false;
  settings.show_right_legend = true;
  settings.show_right_latest_values = true;

  RenderableSeries left;
  left.label = "Left";
  left.color = QColor(250, 40, 40);
  left.axis = SeriesAxis::Left;
  left.samples = std::vector<PlotSample>{{10.0, 0.25}};

  RenderableSeries right;
  right.label = "Right";
  right.color = QColor(40, 120, 255);
  right.axis = SeriesAxis::Right;
  right.samples = std::vector<PlotSample>{{10.0, -0.5}};

  const QImage with_right_values = renderer.render(settings, {left, right});
  settings.show_right_latest_values = false;
  const QImage without_right_values = renderer.render(settings, {left, right});

  EXPECT_GT(countDifferentPixels(with_right_values, without_right_values), 0);
}

TEST(Plot2DRenderer, RightLegendCanMergeWithLeftLegend)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.right_y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_right_y_min = -1.0;
  settings.fixed_right_y_max = 1.0;
  settings.legend_position = LegendPosition::TopLeft;
  settings.right_legend_position = LegendPosition::BottomRight;
  settings.show_latest_values = false;
  settings.show_right_legend = true;
  settings.show_right_latest_values = false;

  RenderableSeries left;
  left.label = "Left";
  left.color = QColor(250, 40, 40);
  left.axis = SeriesAxis::Left;

  RenderableSeries right;
  right.label = "Right";
  right.color = QColor(40, 120, 255);
  right.axis = SeriesAxis::Right;

  settings.merge_right_legend_with_left = false;
  const QImage separate = renderer.render(settings, {left, right});
  settings.merge_right_legend_with_left = true;
  const QImage merged = renderer.render(settings, {left, right});

  EXPECT_GT(
    countPixelsCloseToInRect(separate, right.color, QRect(210, 116, 95, 34)),
    0);
  EXPECT_EQ(
    countPixelsCloseToInRect(merged, right.color, QRect(210, 116, 95, 34)),
    0);
  EXPECT_GT(
    countPixelsCloseToInRect(merged, right.color, QRect(38, 14, 130, 50)),
    0);
}

TEST(Plot2DRenderer, AppendsLegendUnitOnlyWhenLatestValueIsShown)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries series;
  series.label = "Speed";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{10.0, 1.25}};
  series.unit = "m/s";

  const QImage with_unit = renderer.render(settings, {series});
  series.unit = "";
  const QImage without_unit = renderer.render(settings, {series});
  EXPECT_GT(countDifferentPixels(with_unit, without_unit), 0);

  series.samples.clear();
  series.unit = "m/s";
  const QImage waiting_with_unit = renderer.render(settings, {series});
  series.unit = "";
  const QImage waiting_without_unit = renderer.render(settings, {series});
  EXPECT_EQ(countDifferentPixels(waiting_with_unit, waiting_without_unit), 0);
}

TEST(Plot2DRenderer, AppliesLegendOffsets)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 320;
  settings.height = 160;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;
  settings.legend_position = LegendPosition::TopLeft;
  settings.legend_x_offset = 40;
  settings.legend_y_offset = 24;

  RenderableSeries series;
  series.label = "L";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{10.0, 0.0}};

  const QImage image = renderer.render(settings, {series});

  EXPECT_EQ(
    countPixelsCloseToInRect(image, QColor(250, 40, 40), QRect(42, 16, 70, 20)),
    0);
  EXPECT_GT(
    countPixelsCloseToInRect(image, QColor(250, 40, 40), QRect(78, 38, 70, 24)),
    0);
}

TEST(Plot2DRenderer, SpacesLegendRowsFromConfiguredFontMetrics)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings settings;
  settings.width = 360;
  settings.height = 180;
  settings.now = 10.0;
  settings.window_seconds = 5.0;
  settings.font_size = 16;
  settings.show_latest_values = false;
  settings.show_major_grid = false;
  settings.show_minor_grid = false;
  settings.y_scale_mode = rviz_2d_plot_plugin::AxisScaleMode::Fixed;
  settings.fixed_y_min = -1.0;
  settings.fixed_y_max = 1.0;

  RenderableSeries first;
  first.label = "First";
  first.color = QColor(250, 40, 40);
  RenderableSeries second;
  second.label = "Second";
  second.color = QColor(40, 240, 80);
  RenderableSeries third;
  third.label = "Third";
  third.color = QColor(80, 120, 255);

  const QImage image = renderer.render(settings, {first, second, third});
  const QRect first_bounds = coloredPixelBounds(image, first.color);
  const QRect second_bounds = coloredPixelBounds(image, second.color);
  const QRect third_bounds = coloredPixelBounds(image, third.color);

  ASSERT_FALSE(first_bounds.isNull());
  ASSERT_FALSE(second_bounds.isNull());
  ASSERT_FALSE(third_bounds.isNull());

  const QFontMetrics metrics(
    QFont(QStringLiteral("Sans Serif"), std::clamp(settings.font_size, 6, 16)));
  EXPECT_GE(second_bounds.center().y() - first_bounds.center().y(), metrics.height());
  EXPECT_GE(third_bounds.center().y() - second_bounds.center().y(), metrics.height());
}

TEST(Plot2DRenderer, FontSizeChangesTextRendering)
{
  ensureQtApplication();
  Plot2DRenderer renderer;
  PlotRenderSettings small_font;
  small_font.width = 320;
  small_font.height = 160;
  small_font.now = 10.0;
  small_font.window_seconds = 10.0;
  small_font.font_size = 6;

  PlotRenderSettings large_font = small_font;
  large_font.font_size = 14;

  RenderableSeries series;
  series.label = "Text";
  series.color = QColor(250, 40, 40);
  series.samples = std::vector<PlotSample>{{5.0, 0.5}, {10.0, 0.75}};

  const QImage small_image = renderer.render(small_font, {series});
  const QImage large_image = renderer.render(large_font, {series});

  EXPECT_GT(countDifferentPixels(small_image, large_image), 100);
}
