#ifndef RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
#define RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_

#include <rviz_common/display.hpp>

namespace rviz_2d_plot_plugin
{

class Plot2DDisplay : public rviz_common::Display
{
  Q_OBJECT

public:
  Plot2DDisplay();
  ~Plot2DDisplay() override;

protected:
  void onInitialize() override;
  void reset() override;
};

}  // namespace rviz_2d_plot_plugin

#endif  // RVIZ_2D_PLOT_PLUGIN__PLOT_2D_DISPLAY_HPP_
