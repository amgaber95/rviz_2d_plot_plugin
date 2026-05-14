# rviz_2d_plot_plugin

![Version](https://img.shields.io/badge/version-0.2.0-0f172a)
![Humble CI](https://github.com/amgaber95/rviz_2d_plot_plugin/actions/workflows/humble.yml/badge.svg?branch=humble)
![ROS 2 Humble](https://img.shields.io/badge/ROS%202-Humble-2563eb)
![License](https://img.shields.io/badge/license-MIT-16a34a)

A display plugin for RViz 2 that renders live 2D plots as screen-space overlays. It subscribes to ROS 2 topics and discovers plottable fields at runtime, so controller outputs, diagnostics, odometry, and sensor-derived values can stay visible alongside the 3D scene.

![Plot2D overview](docs/images/rviz-2d-plot-overview.png)

## Highlights

| | |
|---|---|
| **Runtime introspection** | Select numeric and boolean fields from active ROS 2 topics at runtime |
| **Time Series & XY** | Plot values over time, or field-vs-field data such as odometry X vs Y |
| **Multi-series overlay** | Combine series from different topics with independent colors, styles, scales, offsets, and units |
| **Reference lines** | Add limits, setpoints, zero lines, and tolerance bands |
| **Row workflow** | Enable rows, drag to reorder, duplicate, or delete series and references |
| **Axis control** | Auto-scale with padding, fixed min/max limits, or locked 1:1 aspect ratio |
| **Rendering styles** | Line / Step / Points with Solid, Dash, Dot, DashDot strokes and fractional widths |
| **Timestamp modes** | Receive time or `std_msgs/Header` stamp with automatic fallback |
| **QoS configuration** | Choose reliability, durability, and queue depth to match publisher QoS |
| **Value transforms** | Per-series scale and offset for unit conversion, such as rad to deg |
| **History preservation** | Samples survive visual edits, reordering, and enable/disable toggles |
| **Pause & Clear** | Freeze updates or reset history without restarting RViz |

## Install

```bash
cd ~/ros2_ws/src
git clone https://github.com/amgaber95/rviz_2d_plot_plugin.git
cd ~/ros2_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select rviz_2d_plot_plugin
source install/setup.bash
```

## Usage

1. **Add** → **Plot2D** in RViz 2
2. Set **Topic** and **Field** for time-series, or switch **Plot Mode** to XY and set **X Field** / **Y Field**
3. Add series via **Series Count**, reorder by dragging in the property tree
4. Tune axes, grid, references, legend, layout, and style as needed

## Properties Reference

<details>
<summary><b>Series</b></summary>

| Property | Description |
|---|---|
| Topic | Dropdown of active ROS 2 topics |
| Field | Nested message path, e.g. `angular_velocity/z` (auto-discovered at runtime) |
| X Field / Y Field | Independent field paths for XY mode |
| Label | Legend display name (also shown when row is collapsed) |
| Unit | Appended to legend values, e.g. `m/s`, `deg` |
| Color | RGB line color |
| Line Width | Stroke width in pixels (supports fractional values) |
| Line Alpha | Opacity 0.0–1.0 |
| Line Style | Solid · Dash · Dot · DashDot |
| Plot Style | Line · Step · Points |
| Value Scale | Multiply raw value before plotting |
| Value Offset | Add after scaling |

Row actions: checkbox enable/disable, drag reorder, duplicate, delete.

</details>

<details>
<summary><b>References</b></summary>

| Property | Description |
|---|---|
| Preset | None · Zero Line · Upper Limit · Lower Limit |
| Preset Value | Y position used when applying a preset |
| Tolerance | Shaded band ± this value around the reference line |
| Apply Preset | Append a new reference from the selected preset |
| Y Value | Y position of the horizontal line |
| Label | Legend text |
| Color | RGB line color |
| Alpha | Opacity 0.0–1.0 |
| Line Width | Stroke width in pixels (supports fractional values) |
| Line Style | Solid · Dash · Dot · DashDot |

Row actions: same workflow as series, with checkbox, drag, duplicate, and delete controls.

</details>

<details>
<summary><b>Time</b></summary>

| Property | Description |
|---|---|
| Window Seconds | Rolling time window width |
| Refresh Rate | Render frequency in Hz (default 20) |
| Time Source | Receive Time · Message Header Stamp |
| XY History Mode | Rolling Time Window · All Samples |

Falls back to receive time when the message has no `std_msgs/Header`.

</details>

<details>
<summary><b>Axes</b></summary>

| Property | Description |
|---|---|
| Auto Scale | Fit to visible data with configurable padding |
| X Min / X Max | Fixed X-axis limits (when auto-scale is off) |
| Y Min / Y Max | Fixed Y-axis limits (when auto-scale is off) |
| Axis Scale | Independent · 1:1 (preserves aspect ratio in XY mode) |

</details>

<details>
<summary><b>Grid</b></summary>

| Property | Description |
|---|---|
| Major Grid | Toggle major grid lines |
| Minor Grid | Toggle minor grid lines |
| X Major Ticks | Target major tick count on X axis |
| Y Major Ticks | Target major tick count on Y axis |
| Minor Divisions | Subdivisions between major ticks |

</details>

<details>
<summary><b>Legend</b></summary>

| Property | Description |
|---|---|
| Enabled | Show or hide the legend |
| Position | Top Left · Top Right · Bottom Left · Bottom Right |
| Show Values | Display latest numeric value next to each series label |
| X Offset / Y Offset | Pixel inset from the chosen corner |

Values include the optional series unit when configured.

</details>

<details>
<summary><b>Layout</b></summary>

| Property | Description |
|---|---|
| Width / Height | Plot dimensions in pixels |
| H Alignment | Left · Center · Right |
| V Alignment | Top · Center · Bottom |
| X Offset / Y Offset | Pixels from the anchor edge |

</details>

<details>
<summary><b>Style</b></summary>

| Property | Description |
|---|---|
| Background Color | Plot background RGB |
| Background Alpha | Plot background opacity |
| Axis Color | Axis lines and tick labels |
| Grid Color | Grid lines |
| Text Color | Legend and value text |
| Font Size | Points |

</details>

<details>
<summary><b>QoS</b></summary>

| Property | Description |
|---|---|
| Reliability | System Default · Reliable · Best Effort |
| Durability | System Default · Volatile · Transient Local |
| Depth | Keep-last subscription queue depth |

</details>

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

[MIT](LICENSE) (c) 2026 Abdelrahman Mahmoud

## Maintainer

**Abdelrahman Mahmoud**

Email: abdulrahman.mahmoud1995@gmail.com  
GitHub: [@amgaber95](https://github.com/amgaber95)
