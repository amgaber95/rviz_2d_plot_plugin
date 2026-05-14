^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rviz_2d_plot_plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.2.0 (2026-05-14)
------------------
* Replace the third-party RViz overlay dependency with an internal overlay backend.
* Add configurable subscription QoS for reliability, durability, and queue depth.
* Add drag reordering for series rows.
* Add explicit duplicate and delete controls for configured series.
* Show configured series sources in collapsed rows and prefer labels when available.
* Move series enable state to the row checkbox for a cleaner property tree.
* Preserve plot history across series reordering and series enable/disable toggles.
* Avoid controller resets for render-only property edits.
* Avoid self-queued refresh renders.
* Support subpixel plot line widths.
* Align reference rows with series-style controls.
* Split Plot2D display tests by concern and add overlay backend coverage.
* Refactor display options, property helpers, and subscription management into focused helpers.

0.1.0 (2026-05-11)
------------------
* Initial release.
* Add time-series and XY plotting for ROS 2 topic fields.
* Add multi-series configuration, runtime field selection, and reference line support.
* Add controls for axes, grid, layout, styling, legends, and tolerance bands.
* Add RViz overlay integration, persisted display configuration, tests, and GitHub Actions CI.
