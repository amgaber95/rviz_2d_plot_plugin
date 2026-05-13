// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_2d_renderer.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>

#include <algorithm>
#include <cmath>

#include "rviz_2d_plot_plugin/plot_range.hpp"
#include "rviz_2d_plot_plugin/plot_value_formatter.hpp"
#include "rviz_2d_plot_plugin/tick_generator.hpp"

namespace rviz_2d_plot_plugin
{
namespace
{

constexpr int kMinimumWidth = 120;
constexpr int kMinimumHeight = 80;

double repairedLineWidth(const double line_width)
{
  if (!std::isfinite(line_width)) {
    return 1.0;
  }
  return std::clamp(line_width, kMinimumLineWidth, kMaximumLineWidth);
}

double majorTickStep(const std::vector<double> & ticks)
{
  if (ticks.size() < 2) {
    return 0.0;
  }
  return std::abs(ticks[1] - ticks[0]);
}

double yAxisLabelWidth(
  const PlotRenderSettings & settings,
  const PlotRange & y_range,
  const QFontMetrics & font_metrics)
{
  const std::size_t y_major_count = static_cast<std::size_t>(
    std::clamp(settings.y_major_tick_count, 2, 20));
  const TickSet ticks = generateTicks(y_range, y_major_count, 0);
  const double step = majorTickStep(ticks.major);

  int width = 0;
  for (const double tick : ticks.major) {
    width = std::max(
      width,
      font_metrics.horizontalAdvance(QString::fromStdString(formatAxisTickValue(tick, step))));
  }
  return static_cast<double>(width);
}

QRectF plotRect(
  const PlotRenderSettings & settings,
  const PlotRange & y_range,
  const QFontMetrics & font_metrics)
{
  const double left_margin = std::clamp(
    yAxisLabelWidth(settings, y_range, font_metrics) + 8.0,
    26.0,
    60.0);
  const double top_margin = std::max(12.0, static_cast<double>(font_metrics.height()));
  const double right_margin = 12.0;
  const double bottom_margin = std::max(22.0, static_cast<double>(font_metrics.height()) + 8.0);

  return QRectF(
    left_margin,
    top_margin,
    static_cast<double>(settings.width) - left_margin - right_margin,
    static_cast<double>(settings.height) - top_margin - bottom_margin);
}

double clampedRatio(const double value)
{
  return std::clamp(value, 0.0, 1.0);
}

double mapX(const QRectF & rect, const PlotRange & range, const double value)
{
  return rect.left() + clampedRatio((value - range.min) / range.span()) * rect.width();
}

double mapTimeAgeX(const QRectF & rect, const double window_seconds, const double age_seconds)
{
  return rect.right() - clampedRatio(age_seconds / window_seconds) * rect.width();
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

void applyEqualXYScale(
  const QRectF & rect,
  PlotRange & x_range,
  PlotRange & y_range)
{
  if (!x_range.isValid() || !y_range.isValid() || rect.width() <= 0.0 || rect.height() <= 0.0) {
    return;
  }

  const double units_per_pixel = std::max(
    x_range.span() / rect.width(),
    y_range.span() / rect.height());
  const double x_center = (x_range.min + x_range.max) * 0.5;
  const double y_center = (y_range.min + y_range.max) * 0.5;
  const double x_span = units_per_pixel * rect.width();
  const double y_span = units_per_pixel * rect.height();
  x_range = PlotRange{x_center - x_span * 0.5, x_center + x_span * 0.5};
  y_range = PlotRange{y_center - y_span * 0.5, y_center + y_span * 0.5};
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
  const TickSet x_ticks = settings.x_axis_mode == XAxisMode::Time ?
    generateTicks(PlotRange{0.0, settings.window_seconds}, x_major_count, minor_divisions) :
    generateTicks(x_range, x_major_count, minor_divisions);
  const TickSet y_ticks = generateTicks(y_range, y_major_count, minor_divisions);
  const double x_step = majorTickStep(x_ticks.major);
  const double y_step = majorTickStep(y_ticks.major);

  if (settings.show_minor_grid) {
    painter.setPen(QPen(scaledAlpha(settings.grid_color, 0.45), 1.0));
    for (const double tick : y_ticks.minor) {
      const double y = mapY(rect, y_range, tick);
      painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
    for (const double tick : x_ticks.minor) {
      const double x = settings.x_axis_mode == XAxisMode::Time ?
        mapTimeAgeX(rect, settings.window_seconds, tick) :
        mapX(rect, x_range, tick);
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
      const double x = settings.x_axis_mode == XAxisMode::Time ?
        mapTimeAgeX(rect, settings.window_seconds, tick) :
        mapX(rect, x_range, tick);
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
      QString::fromStdString(formatAxisTickValue(tick, y_step)));
  }

  for (const double tick : x_ticks.major) {
    const double x = settings.x_axis_mode == XAxisMode::Time ?
      mapTimeAgeX(rect, settings.window_seconds, tick) :
      mapX(rect, x_range, tick);
    const QString label = settings.x_axis_mode == XAxisMode::Time ?
      formatTimeOffset(-tick) :
      QString::fromStdString(formatAxisTickValue(tick, x_step));
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
    repairedLineWidth(series.line_width),
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
    const double radius = std::max(2.0, repairedLineWidth(series.line_width) * 1.5);
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
    if (!reference.enabled) {
      continue;
    }

    const double tolerance = std::isfinite(reference.tolerance) ?
      std::max(0.0, reference.tolerance) : 0.0;
    if (tolerance > 0.0) {
      const double lower = reference.value - tolerance;
      const double upper = reference.value + tolerance;
      if (upper >= y_range.min && lower <= y_range.max) {
        const double band_min = std::max(lower, y_range.min);
        const double band_max = std::min(upper, y_range.max);
        const double top = mapY(rect, y_range, band_max);
        const double bottom = mapY(rect, y_range, band_min);
        QColor fill_color = reference.color;
        fill_color.setAlpha(std::clamp(reference.color.alpha() / 5, 12, 48));
        painter.fillRect(
          QRectF(rect.left(), top, rect.width(), std::max(1.0, bottom - top)),
          fill_color);
      }
    }

    if (reference.value >= y_range.min && reference.value <= y_range.max) {
      const double y = mapY(rect, y_range, reference.value);
      painter.setPen(
        QPen(
          reference.color,
          repairedLineWidth(reference.line_width),
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
}

void drawLegend(
  QPainter & painter,
  const QRectF & rect,
  const std::vector<RenderableSeries> & series,
  const PlotRange & x_range,
  const PlotRenderSettings & settings)
{
  if (!settings.show_legend) {
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
    if (!item.enabled) {
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

    QString text = QString::fromStdString(item.label);
    if (settings.show_latest_values && latest != item.samples.rend()) {
      text += " ";
      text += QString::fromStdString(formatPlotValue(latest->value));
      if (!item.unit.empty()) {
        text += " ";
        text += QString::fromStdString(item.unit);
      }
    }
    text_width = std::max(
      text_width,
      static_cast<double>(painter.fontMetrics().horizontalAdvance(text)));
    entries.push_back(LegendEntry{item.color, text});
  }

  if (entries.empty()) {
    return;
  }

  const double line_height = std::max(
    14.0,
    static_cast<double>(painter.fontMetrics().height()) + 2.0);
  const double legend_width = std::min(rect.width() - 8.0, std::max(72.0, text_width + 24.0));
  const double legend_height = line_height * static_cast<double>(entries.size());
  const bool align_right =
    settings.legend_position == LegendPosition::TopRight ||
    settings.legend_position == LegendPosition::BottomRight;
  const bool align_bottom =
    settings.legend_position == LegendPosition::BottomLeft ||
    settings.legend_position == LegendPosition::BottomRight;
  const double x_offset = static_cast<double>(std::max(0, settings.legend_x_offset));
  const double y_offset = static_cast<double>(std::max(0, settings.legend_y_offset));
  const double x = align_right ? rect.right() - legend_width - x_offset : rect.left() + x_offset;
  double y = align_bottom ? rect.bottom() - legend_height - y_offset : rect.top() + y_offset;

  for (const LegendEntry & entry : entries) {
    const double row_center = y + line_height * 0.5;
    painter.setPen(QPen(entry.color, 2.0));
    painter.drawLine(QPointF(x, row_center), QPointF(x + 16.0, row_center));

    painter.setPen(QPen(settings.text_color, 1.0));
    painter.drawText(
      QRectF(x + 20.0, y, legend_width - 20.0, line_height),
      Qt::AlignLeft | Qt::AlignVCenter,
      entry.text);
    y += line_height;
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

  const PlotRange time_range{
    settings.now - settings.window_seconds,
    settings.now};
  const std::vector<PlotSample> samples = visibleSamples(
    series, time_range, settings.x_axis_mode);
  PlotRange x_range = xRangeForSettings(settings, samples);
  PlotRange y_range = yRangeForSettings(settings, samples);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(QFont(QStringLiteral("Sans Serif"), std::clamp(settings.font_size, 6, 16)));

  QRectF rect = plotRect(settings, y_range, painter.fontMetrics());
  if (settings.x_axis_mode == XAxisMode::Field &&
    settings.xy_axis_scale_mode == XYAxisScaleMode::Equal)
  {
    applyEqualXYScale(rect, x_range, y_range);
    rect = plotRect(settings, y_range, painter.fontMetrics());
  }

  drawGrid(painter, rect, x_range, y_range, settings);
  drawReferences(painter, rect, y_range, references, settings);
  for (const RenderableSeries & item : series) {
    drawSeries(painter, rect, x_range, y_range, item);
  }
  drawLegend(painter, rect, series, x_range, settings);
  return image;
}

}  // namespace rviz_2d_plot_plugin
