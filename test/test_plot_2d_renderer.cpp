// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QRect>

#include <gtest/gtest.h>

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
      std::vector<PlotSample>{{8.0, 1.0}, {9.0, 2.0}, {10.0, 1.5}}}};

  const QImage image = renderer.render(settings, series);

  EXPECT_TRUE(hasDifferentPixel(image, settings.background_color));
  EXPECT_NE(image.pixelColor(1, 1), QColor(Qt::transparent));
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
