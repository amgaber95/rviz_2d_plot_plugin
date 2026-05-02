// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_renderer.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "rviz_2d_plot_plugin/plot_range.hpp"
#include "rviz_2d_plot_plugin/tick_generator.hpp"

namespace rviz_2d_plot_plugin
{
namespace
{

constexpr int kMinimumWidth = 120;
constexpr int kMinimumHeight = 80;

QRectF plotRect(const PlotRenderSettings & settings)
{
  return QRectF(
    42.0,
    16.0,
    static_cast<double>(settings.width - 54),
    static_cast<double>(settings.height - 44));
}

double clampedRatio(const double value)
{
  return std::clamp(value, 0.0, 1.0);
}

double mapX(const QRectF & rect, const PlotRange & range, const double value)
{
  return rect.left() + clampedRatio((value - range.min) / range.span()) * rect.width();
}

double mapY(const QRectF & rect, const PlotRange & range, const double value)
{
  return rect.bottom() - clampedRatio((value - range.min) / range.span()) * rect.height();
}

std::vector<PlotSample> visibleSamples(
  const std::vector<RenderableSeries> & series,
  const PlotRange & time_range)
{
  std::vector<PlotSample> samples;
  for (const RenderableSeries & item : series) {
    if (!item.enabled) {
      continue;
    }
    for (const PlotSample & sample : item.samples) {
      if (sample.time >= time_range.min && sample.time <= time_range.max) {
        samples.push_back(sample);
      }
    }
  }
  return samples;
}

PlotRange yRangeForSettings(
  const PlotRenderSettings & settings,
  const std::vector<PlotSample> & samples)
{
  if (settings.y_scale_mode == AxisScaleMode::Fixed) {
    return makeFixedRange(settings.fixed_y_min, settings.fixed_y_max);
  }
  return makeAutoRange(samples, settings.y_padding_fraction);
}

std::string formatValue(const double value)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

void drawGrid(
  QPainter & painter,
  const QRectF & rect,
  const PlotRange & x_range,
  const PlotRange & y_range,
  const PlotRenderSettings & settings)
{
  painter.setPen(QPen(settings.grid_color, 1.0));

  const TickSet y_ticks = generateTicks(y_range, 5, 0);
  for (const double tick : y_ticks.major) {
    const double y = mapY(rect, y_range, tick);
    painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
  }

  for (int i = 0; i <= 5; ++i) {
    const double x = rect.left() + rect.width() * static_cast<double>(i) / 5.0;
    painter.drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
  }

  painter.setPen(QPen(settings.axis_color, 1.0));
  painter.drawRect(rect);

  painter.setPen(QPen(settings.text_color, 1.0));
  for (const double tick : y_ticks.major) {
    const double y = mapY(rect, y_range, tick);
    painter.drawText(
      QRectF(2.0, y - 8.0, rect.left() - 6.0, 16.0),
      Qt::AlignRight | Qt::AlignVCenter,
      QString::fromStdString(formatValue(tick)));
  }

  painter.drawText(
    QRectF(rect.left(), rect.bottom() + 4.0, rect.width(), 18.0),
    Qt::AlignLeft | Qt::AlignVCenter,
    QString::number(x_range.min - settings.now, 'f', 0) + "s");
  painter.drawText(
    QRectF(rect.left(), rect.bottom() + 4.0, rect.width(), 18.0),
    Qt::AlignRight | Qt::AlignVCenter,
    "now");
}

void drawSeries(
  QPainter & painter,
  const QRectF & rect,
  const PlotRange & x_range,
  const PlotRange & y_range,
  const RenderableSeries & series)
{
  if (!series.enabled || series.samples.empty()) {
    return;
  }

  QPainterPath path;
  bool has_point = false;
  for (const PlotSample & sample : series.samples) {
    if (sample.time < x_range.min || sample.time > x_range.max) {
      continue;
    }
    const QPointF point(
      mapX(rect, x_range, sample.time),
      mapY(rect, y_range, sample.value));
    if (!has_point) {
      path.moveTo(point);
      has_point = true;
    } else {
      path.lineTo(point);
    }
  }

  if (!has_point) {
    return;
  }

  painter.setPen(QPen(series.color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.drawPath(path);
}

void drawLegend(
  QPainter & painter,
  const QRectF & rect,
  const std::vector<RenderableSeries> & series,
  const PlotRange & x_range,
  const PlotRenderSettings & settings)
{
  double y = rect.top() + 4.0;
  for (const RenderableSeries & item : series) {
    if (!item.enabled || item.samples.empty()) {
      continue;
    }

    auto latest = std::find_if(
      item.samples.rbegin(), item.samples.rend(),
      [&x_range](const PlotSample & sample) {
        return sample.time >= x_range.min && sample.time <= x_range.max;
      });
    if (latest == item.samples.rend()) {
      continue;
    }

    painter.setPen(QPen(item.color, 2.0));
    painter.drawLine(QPointF(rect.left() + 8.0, y + 7.0), QPointF(rect.left() + 26.0, y + 7.0));

    painter.setPen(QPen(settings.text_color, 1.0));
    const QString text = QString::fromStdString(item.label + " " + formatValue(latest->value));
    painter.drawText(QRectF(rect.left() + 32.0, y, rect.width() - 36.0, 16.0), text);
    y += 16.0;
  }
}

}  // namespace

QImage Plot2DRenderer::render(
  PlotRenderSettings settings,
  const std::vector<RenderableSeries> & series) const
{
  settings.width = std::max(settings.width, kMinimumWidth);
  settings.height = std::max(settings.height, kMinimumHeight);
  settings.window_seconds = std::max(settings.window_seconds, 1.0);

  QImage image(
    settings.width,
    settings.height,
    QImage::Format_ARGB32_Premultiplied);
  image.fill(settings.background_color);

  const QRectF rect = plotRect(settings);
  const PlotRange x_range{
    settings.now - settings.window_seconds,
    settings.now};
  const std::vector<PlotSample> samples = visibleSamples(series, x_range);
  const PlotRange y_range = yRangeForSettings(settings, samples);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  drawGrid(painter, rect, x_range, y_range, settings);
  for (const RenderableSeries & item : series) {
    drawSeries(painter, rect, x_range, y_range, item);
  }
  drawLegend(painter, rect, series, x_range, settings);
  return image;
}

}  // namespace rviz_2d_plot_plugin
