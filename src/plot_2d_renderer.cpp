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
  const PlotRange & y_range,
  const int major_tick_count,
  const QFontMetrics & font_metrics)
{
  const std::size_t y_major_count = static_cast<std::size_t>(
    std::clamp(major_tick_count, 2, 20));
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

double leftYAxisLabelWidth(
  const PlotRenderSettings & settings,
  const PlotRange & y_range,
  const QFontMetrics & font_metrics)
{
  return yAxisLabelWidth(y_range, settings.y_major_tick_count, font_metrics);
}

double rightYAxisLabelWidth(
  const PlotRenderSettings & settings,
  const PlotRange & right_y_range,
  const bool has_right_axis,
  const QFontMetrics & font_metrics)
{
  if (!has_right_axis) {
    return 0.0;
  }
  return yAxisLabelWidth(right_y_range, settings.y_major_tick_count, font_metrics);
}

QRectF plotRect(
  const PlotRenderSettings & settings,
  const PlotRange & left_y_range,
  const PlotRange & right_y_range,
  const bool has_right_axis,
  const QFontMetrics & font_metrics)
{
  const double left_margin = std::clamp(
    leftYAxisLabelWidth(settings, left_y_range, font_metrics) + 8.0,
    26.0,
    60.0);
  const double top_margin = std::max(12.0, static_cast<double>(font_metrics.height()));
  const double right_margin = std::clamp(
    rightYAxisLabelWidth(settings, right_y_range, has_right_axis, font_metrics) + 8.0,
    12.0,
    60.0);
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

std::vector<PlotSample> visibleSamplesForAxis(
  const std::vector<RenderableSeries> & series,
  const PlotRange & time_range,
  const XAxisMode x_axis_mode,
  const SeriesAxis axis)
{
  std::vector<PlotSample> samples;
  for (const RenderableSeries & item : series) {
    if (!item.enabled || item.axis != axis) {
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

bool hasEnabledSeriesOnAxis(
  const std::vector<RenderableSeries> & series,
  const SeriesAxis axis)
{
  return std::any_of(
    series.begin(), series.end(),
    [axis](const RenderableSeries & item) {
      return item.enabled && item.axis == axis;
    });
}

PlotRange yRangeForAxis(
  const AxisScaleMode scale_mode,
  const double fixed_min,
  const double fixed_max,
  const double padding_fraction,
  const std::vector<PlotSample> & samples)
{
  if (scale_mode == AxisScaleMode::Fixed) {
    return makeFixedRange(fixed_min, fixed_max);
  }
  return makeAutoRange(samples, padding_fraction);
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

std::string terminalFieldToken(const std::string & field_name)
{
  if (field_name.empty()) {
    return {};
  }

  const std::size_t end = field_name.find_last_not_of('/');
  if (end == std::string::npos) {
    return {};
  }

  const std::size_t begin = field_name.find_last_of('/', end);
  const std::size_t start = begin == std::string::npos ? 0 : begin + 1;
  return field_name.substr(start, end - start + 1);
}

void drawGrid(
  QPainter & painter,
  const QRectF & rect,
  const PlotRange & x_range,
  const PlotRange & left_y_range,
  const PlotRange & right_y_range,
  const bool has_right_axis,
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
  const TickSet left_y_ticks = generateTicks(left_y_range, y_major_count, minor_divisions);
  const TickSet right_y_ticks = generateTicks(right_y_range, y_major_count, minor_divisions);
  const double x_step = majorTickStep(x_ticks.major);
  const double left_y_step = majorTickStep(left_y_ticks.major);
  const double right_y_step = majorTickStep(right_y_ticks.major);

  if (settings.show_minor_grid) {
    painter.setPen(QPen(scaledAlpha(settings.grid_color, 0.45), 1.0));
    for (const double tick : left_y_ticks.minor) {
      const double y = mapY(rect, left_y_range, tick);
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
    for (const double tick : left_y_ticks.major) {
      const double y = mapY(rect, left_y_range, tick);
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
  for (const double tick : left_y_ticks.major) {
    const double y = mapY(rect, left_y_range, tick);
    painter.drawText(
      QRectF(2.0, y - 8.0, rect.left() - 6.0, 16.0),
      Qt::AlignRight | Qt::AlignVCenter,
      QString::fromStdString(formatAxisTickValue(tick, left_y_step)));
  }

  if (has_right_axis) {
    for (const double tick : right_y_ticks.major) {
      const double y = mapY(rect, right_y_range, tick);
      painter.drawText(
        QRectF(rect.right() + 6.0, y - 8.0, settings.width - rect.right() - 8.0, 16.0),
        Qt::AlignLeft | Qt::AlignVCenter,
        QString::fromStdString(formatAxisTickValue(tick, right_y_step)));
    }
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

struct LegendEntry
{
  QColor color;
  QString text;
};

void appendLegendEntries(
  const std::vector<RenderableSeries> & series,
  const PlotRange & x_range,
  const PlotRenderSettings & settings,
  const bool filter_axis,
  const SeriesAxis axis,
  const bool show_values,
  std::vector<LegendEntry> & entries,
  const QString & axis_prefix = "")
{
  for (const RenderableSeries & item : series) {
    if (!item.enabled || (filter_axis && item.axis != axis)) {
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
    if (settings.legend_field_name_only) {
      const std::string field_token = terminalFieldToken(item.field_name);
      if (!field_token.empty()) {
        text = QString::fromStdString(field_token);
      }
    }
    if (!axis_prefix.isEmpty()) {
      text = axis_prefix + " " + text;
    }
    if (show_values && latest != item.samples.rend()) {
      text += " ";
      text += QString::fromStdString(formatPlotValue(latest->value));
      if (!item.unit.empty()) {
        text += " ";
        text += QString::fromStdString(item.unit);
      }
    }
    entries.push_back(LegendEntry{item.color, text});
  }
}

void drawLegendEntries(
  QPainter & painter,
  const QRectF & rect,
  const PlotRenderSettings & settings,
  const std::vector<LegendEntry> & entries,
  const LegendPosition position,
  const int x_offset_px,
  const int y_offset_px)
{
  if (entries.empty()) {
    return;
  }

  double text_width = 0.0;
  for (const LegendEntry & entry : entries) {
    text_width = std::max(
      text_width,
      static_cast<double>(painter.fontMetrics().horizontalAdvance(entry.text)));
  }

  const double line_height = std::max(
    14.0,
    static_cast<double>(painter.fontMetrics().height()) + 2.0);
  const double legend_width = std::min(rect.width() - 8.0, std::max(72.0, text_width + 24.0));
  const double legend_height = line_height * static_cast<double>(entries.size());
  const bool align_right =
    position == LegendPosition::TopRight ||
    position == LegendPosition::BottomRight;
  const bool align_bottom =
    position == LegendPosition::BottomLeft ||
    position == LegendPosition::BottomRight;
  const double x_offset = static_cast<double>(std::max(0, x_offset_px));
  const double y_offset = static_cast<double>(std::max(0, y_offset_px));
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

  std::vector<LegendEntry> left_entries;
  std::vector<LegendEntry> right_entries;
  left_entries.reserve(series.size());
  right_entries.reserve(series.size());

  appendLegendEntries(
    series,
    x_range,
    settings,
    true,
    SeriesAxis::Left,
    settings.show_latest_values,
    left_entries);
  if (settings.show_right_legend) {
    appendLegendEntries(
      series,
      x_range,
      settings,
      true,
      SeriesAxis::Right,
      settings.show_right_latest_values,
      right_entries);
  }

  if (settings.merge_right_legend_with_left && settings.show_right_legend) {
    std::vector<LegendEntry> merged_entries;
    merged_entries.reserve(left_entries.size() + right_entries.size());
    appendLegendEntries(
      series,
      x_range,
      settings,
      true,
      SeriesAxis::Left,
      settings.show_latest_values,
      merged_entries,
      "[L]");
    appendLegendEntries(
      series,
      x_range,
      settings,
      true,
      SeriesAxis::Right,
      settings.show_right_latest_values,
      merged_entries,
      "[R]");
    drawLegendEntries(
      painter,
      rect,
      settings,
      merged_entries,
      settings.legend_position,
      settings.legend_x_offset,
      settings.legend_y_offset);
    return;
  }

  drawLegendEntries(
    painter,
    rect,
    settings,
    left_entries,
    settings.legend_position,
    settings.legend_x_offset,
    settings.legend_y_offset);
  drawLegendEntries(
    painter,
    rect,
    settings,
    right_entries,
    settings.right_legend_position,
    settings.right_legend_x_offset,
    settings.right_legend_y_offset);
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
  const std::vector<PlotSample> left_axis_samples = visibleSamplesForAxis(
    series, time_range, settings.x_axis_mode, SeriesAxis::Left);
  const std::vector<PlotSample> right_axis_samples = visibleSamplesForAxis(
    series, time_range, settings.x_axis_mode, SeriesAxis::Right);
  const bool has_right_axis = hasEnabledSeriesOnAxis(series, SeriesAxis::Right);
  PlotRange x_range = xRangeForSettings(settings, samples);
  PlotRange left_y_range = yRangeForAxis(
    settings.y_scale_mode,
    settings.fixed_y_min,
    settings.fixed_y_max,
    settings.y_padding_fraction,
    left_axis_samples.empty() ? samples : left_axis_samples);
  PlotRange right_y_range = has_right_axis ?
    yRangeForAxis(
      settings.right_y_scale_mode,
      settings.fixed_right_y_min,
      settings.fixed_right_y_max,
      settings.right_y_padding_fraction,
      right_axis_samples) : left_y_range;

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(QFont(QStringLiteral("Sans Serif"), std::clamp(settings.font_size, 6, 16)));

  QRectF rect = plotRect(
    settings,
    left_y_range,
    right_y_range,
    has_right_axis,
    painter.fontMetrics());
  if (settings.x_axis_mode == XAxisMode::Field &&
    settings.xy_axis_scale_mode == XYAxisScaleMode::Equal &&
    !has_right_axis)
  {
    applyEqualXYScale(rect, x_range, left_y_range);
    rect = plotRect(
      settings,
      left_y_range,
      right_y_range,
      has_right_axis,
      painter.fontMetrics());
  }

  drawGrid(painter, rect, x_range, left_y_range, right_y_range, has_right_axis, settings);
  drawReferences(painter, rect, left_y_range, references, settings);
  for (const RenderableSeries & item : series) {
    const PlotRange & series_y_range =
      item.axis == SeriesAxis::Right ? right_y_range : left_y_range;
    drawSeries(painter, rect, x_range, series_y_range, item);
  }
  drawLegend(painter, rect, series, x_range, settings);
  return image;
}

}  // namespace rviz_2d_plot_plugin
