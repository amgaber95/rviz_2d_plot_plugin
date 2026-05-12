// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OVERLAY_BACKEND_HPP_
#define OVERLAY_BACKEND_HPP_

#include <memory>
#include <string>

class QImage;

namespace Ogre
{
class SceneManager;
}  // namespace Ogre

namespace rviz_2d_plot_plugin
{

enum class OverlayHorizontalAlignment
{
  Left,
  Center,
  Right,
};

enum class OverlayVerticalAlignment
{
  Top,
  Center,
  Bottom,
};

struct OverlayGeometry
{
  int width{360};
  int height{220};
  int x_offset{0};
  int y_offset{0};
  OverlayHorizontalAlignment horizontal_alignment{OverlayHorizontalAlignment::Right};
  OverlayVerticalAlignment vertical_alignment{OverlayVerticalAlignment::Top};
};

struct OverlaySize
{
  unsigned int width{1};
  unsigned int height{1};
};

struct OverlayPosition
{
  double left{0.0};
  double top{0.0};
};

enum class OverlayBackendStatus
{
  Ok,
  Warning,
  Error,
};

struct OverlayBackendResult
{
  OverlayBackendStatus status{OverlayBackendStatus::Ok};
  std::string message;

  bool ok() const
  {
    return status != OverlayBackendStatus::Error;
  }
};

OverlaySize clampedOverlaySize(int width, int height);

OverlayPosition alignedOverlayPosition(const OverlayGeometry & geometry);

class OverlayBackend
{
public:
  virtual ~OverlayBackend() = default;

  virtual OverlayBackendResult initialize(Ogre::SceneManager * scene_manager) = 0;
  virtual OverlayBackendResult setGeometry(const OverlayGeometry & geometry) = 0;
  virtual void setVisible(bool visible) = 0;
  virtual bool isReady() const = 0;
  virtual OverlayBackendResult updateImage(const QImage & image) = 0;
};

std::unique_ptr<OverlayBackend> makeOgreOverlayBackend(std::string name);

}  // namespace rviz_2d_plot_plugin

#endif  // OVERLAY_BACKEND_HPP_
