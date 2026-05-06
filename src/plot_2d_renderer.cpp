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
  const PlotRange & time_range,
  const XAxisMode x_axis_mode)
{
  std::vector<PlotSample> samples;
  for (const RenderableSeries & item : series) {
    if (!item.enabled) {
      continue;
    }
    for (const PlotSample & sample : item.samples) {
      if (x_axis_mode == XAxisMode::Field ||
        (sample.time >= time_range.min && sample.time <= time_range.max))
      {
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

PlotRange xRangeForSettings(
  const PlotRenderSettings & settings,
  const std::vector<PlotSample> & samples)
{
  if (settings.x_axis_mode == XAxisMode::Time) {
    return PlotRange{settings.now - settings.window_seconds, settings.now};
  }

  if (settings.x_scale_mode == AxisScaleMode::Fixed) {
    return makeFixedRange(settings.fixed_x_min, settings.fixed_x_max);
  }

  std::vector<PlotSample> x_samples;
  x_samples.reserve(samples.size());
  for (const PlotSample & sample : samples) {
    x_samples.push_back(PlotSample{sample.time, sample.x});
  }
  return makeAutoRange(x_samples, settings.x_padding_fraction);
}

std::string formatValue(const double value)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

QString formatTimeOffset(const double seconds)
{
  if (std::abs(seconds) < 1e-6) {
    return "now";
  }
  return QString::number(seconds, 'f', 0) + "s";
}

QColor scaledAlpha(QColor color, const double scale)
{
  color.setAlpha(std::clamp(static_cast<int>(static_cast<double>(color.alpha()) * scale), 0, 255));
  return color;
}

Qt::PenStyle qtPenStyle(const LineStyle style)
{
  switch (style) {
    case LineStyle::Solid:
      return Qt::SolidLine;
    case LineStyle::Dash:
      return Qt::DashLine;
    case LineStyle::Dot:
      return Qt::DotLine;
    case LineStyle::DashDot:
      return Qt::DashDotLine;
  }
  return Qt::SolidLine;
}

void drawGrid(
  QPainter & painter,
  const QRectF & rect,
  const PlotRange & x_range,
  const PlotRange & y_range,
  const PlotRenderSettings & settings)
{
  const std::size_t x_major_count = static_cast<std::size_t>(
    std::clamp(settings.x_major_tick_count, 2, 20));
  const std::size_t y_major_count = static_cast<std::size_t>(
    std::clamp(settings.y_major_tick_count, 2, 20));
  const std::size_t minor_divisions = static_cast<std::size_t>(
    std::clamp(settings.minor_grid_divisions, 0, 8));
  const TickSet x_ticks = generateTicks(x_range, x_major_count, minor_divisions);
  const TickSet y_ticks = generateTicks(y_range, y_major_count, minor_divisions);

  if (settings.show_minor_grid) {
    painter.setPen(QPen(scaledAlpha(settings.grid_color, 0.45), 1.0));
    for (const double tick : y_ticks.minor) {
      const double y = mapY(rect, y_range, tick);
      painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
    for (const double tick : x_ticks.minor) {
      const double x = mapX(rect, x_range, tick);
      painter.drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
  }

  if (settings.show_major_grid) {
    painter.setPen(QPen(settings.grid_color, 1.0));
    for (const double tick : y_ticks.major) {
      const double y = mapY(rect, y_range, tick);
      painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
    for (const double tick : x_ticks.major) {
      const double x = mapX(rect, x_range, tick);
      painter.drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
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

  for (const double tick : x_ticks.major) {
    const double x = mapX(rect, x_range, tick);
    const QString label = settings.x_axis_mode == XAxisMode::Time ?
      formatTimeOffset(tick - settings.now) :
      QString::fromStdString(formatValue(tick));
    painter.drawText(
      QRectF(x - 30.0, rect.bottom() + 4.0, 60.0, 18.0),
      Qt::AlignHCenter | Qt::AlignVCenter,
      label);
  }
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

  QPen pen(
    series.color,
    std::max(1.0, series.line_width),
    qtPenStyle(series.line_style),
    Qt::RoundCap,
    Qt::RoundJoin);
  painter.setPen(pen);

  std::vector<QPointF> points;
  for (const PlotSample & sample : series.samples) {
    if (sample.x < x_range.min || sample.x > x_range.max) {
      continue;
    }
    points.emplace_back(
      mapX(rect, x_range, sample.x),
      mapY(rect, y_range, sample.value));
  }

  if (points.empty()) {
    return;
  }

  if (series.plot_style == PlotStyle::Points) {
    painter.setBrush(series.color);
    const double radius = std::max(2.0, series.line_width * 1.5);
    for (const QPointF & point : points) {
      painter.drawEllipse(point, radius, radius);
    }
    painter.setBrush(Qt::NoBrush);
    return;
  }

  QPainterPath path;
  path.moveTo(points.front());
  for (std::size_t i = 1; i < points.size(); ++i) {
    if (series.plot_style == PlotStyle::Step) {
      path.lineTo(QPointF(points[i].x(), points[i - 1].y()));
    }
    path.lineTo(points[i]);
  }

  painter.drawPath(path);
}

void drawReferences(
  QPainter & painter,
  const QRectF & rect,
  const PlotRange & y_range,
  const std::vector<RenderableReference> & references,
  const PlotRenderSettings & settings)
{
  for (const RenderableReference & reference : references) {
    if (!reference.enabled || reference.value < y_range.min || reference.value > y_range.max) {
      continue;
    }

    const double y = mapY(rect, y_range, reference.value);
    painter.setPen(
      QPen(
        reference.color,
        std::max(1.0, reference.line_width),
        qtPenStyle(reference.line_style),
        Qt::RoundCap,
        Qt::RoundJoin));
    painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));

    if (!reference.label.empty()) {
      painter.setPen(QPen(settings.text_color, 1.0));
      painter.drawText(
        QRectF(rect.left() + 6.0, y - 15.0, rect.width() - 12.0, 14.0),
        Qt::AlignRight | Qt::AlignVCenter,
        QString::fromStdString(reference.label));
    }
  }
}

void drawLegend(
  QPainter & painter,
  const QRectF & rect,
  const std::vector<RenderableSeries> & series,
  const PlotRange & x_range,
  const PlotRenderSettings & settings)
{
  if (settings.legend_position == LegendPosition::Hidden) {
    return;
  }

  struct LegendEntry
  {
    QColor color;
    QString text;
  };

  std::vector<LegendEntry> entries;
  entries.reserve(series.size());
  double text_width = 0.0;
  for (const RenderableSeries & item : series) {
    if (!item.enabled || item.samples.empty()) {
      continue;
    }

    auto latest = std::find_if(
      item.samples.rbegin(), item.samples.rend(),
      [&x_range, &settings](const PlotSample & sample) {
        if (settings.x_axis_mode == XAxisMode::Field) {
          return sample.x >= x_range.min && sample.x <= x_range.max;
        }
        return sample.time >= x_range.min && sample.time <= x_range.max;
      });
    if (latest == item.samples.rend()) {
      continue;
    }

    const QString text = QString::fromStdString(item.label + " " + formatValue(latest->value));
    text_width = std::max(
      text_width,
      static_cast<double>(painter.fontMetrics().horizontalAdvance(text)));
    entries.push_back(LegendEntry{item.color, text});
  }

  if (entries.empty()) {
    return;
  }

  const double line_height = 16.0;
  const double legend_width = std::min(rect.width() - 8.0, std::max(88.0, text_width + 38.0));
  const double legend_height = line_height * static_cast<double>(entries.size());
  const bool align_right =
    settings.legend_position == LegendPosition::TopRight ||
    settings.legend_position == LegendPosition::BottomRight;
  const bool align_bottom =
    settings.legend_position == LegendPosition::BottomLeft ||
    settings.legend_position == LegendPosition::BottomRight;
  const double x = align_right ? rect.right() - legend_width - 4.0 : rect.left() + 4.0;
  double y = align_bottom ? rect.bottom() - legend_height - 4.0 : rect.top() + 4.0;

  for (const LegendEntry & entry : entries) {
    painter.setPen(QPen(entry.color, 2.0));
    painter.drawLine(QPointF(x + 4.0, y + 7.0), QPointF(x + 22.0, y + 7.0));

    painter.setPen(QPen(settings.text_color, 1.0));
    painter.drawText(QRectF(x + 28.0, y, legend_width - 30.0, line_height), entry.text);
    y += 16.0;
  }
}

}  // namespace

QImage Plot2DRenderer::render(
  PlotRenderSettings settings,
  const std::vector<RenderableSeries> & series,
  const std::vector<RenderableReference> & references) const
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
  const PlotRange time_range{
    settings.now - settings.window_seconds,
    settings.now};
  const std::vector<PlotSample> samples = visibleSamples(
    series, time_range, settings.x_axis_mode);
  const PlotRange x_range = xRangeForSettings(settings, samples);
  const PlotRange y_range = yRangeForSettings(settings, samples);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  drawGrid(painter, rect, x_range, y_range, settings);
  drawReferences(painter, rect, y_range, references, settings);
  for (const RenderableSeries & item : series) {
    drawSeries(painter, rect, x_range, y_range, item);
  }
  drawLegend(painter, rect, series, x_range, settings);
  return image;
}

}  // namespace rviz_2d_plot_plugin
