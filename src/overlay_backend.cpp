// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "overlay_backend.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreResourceGroupManager.h>
#include <OgreTechnique.h>
#include <OgreTexture.h>
#include <OgreTextureManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgrePanelOverlayElement.h>
#include <QImage>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <rviz_rendering/render_system.hpp>

namespace rviz_2d_plot_plugin
{
namespace
{

OverlayBackendResult okResult()
{
  return OverlayBackendResult{};
}

OverlayBackendResult errorResult(const std::string & message)
{
  return OverlayBackendResult{OverlayBackendStatus::Error, message};
}

OverlayBackendResult warningResult(const std::string & message)
{
  return OverlayBackendResult{OverlayBackendStatus::Warning, message};
}

Ogre::GuiHorizontalAlignment toOgreHorizontalAlignment(
  const OverlayHorizontalAlignment alignment)
{
  switch (alignment) {
    case OverlayHorizontalAlignment::Left:
      return Ogre::GuiHorizontalAlignment::GHA_LEFT;
    case OverlayHorizontalAlignment::Center:
      return Ogre::GuiHorizontalAlignment::GHA_CENTER;
    case OverlayHorizontalAlignment::Right:
      return Ogre::GuiHorizontalAlignment::GHA_RIGHT;
  }
  return Ogre::GuiHorizontalAlignment::GHA_RIGHT;
}

Ogre::GuiVerticalAlignment toOgreVerticalAlignment(
  const OverlayVerticalAlignment alignment)
{
  switch (alignment) {
    case OverlayVerticalAlignment::Top:
      return Ogre::GuiVerticalAlignment::GVA_TOP;
    case OverlayVerticalAlignment::Center:
      return Ogre::GuiVerticalAlignment::GVA_CENTER;
    case OverlayVerticalAlignment::Bottom:
      return Ogre::GuiVerticalAlignment::GVA_BOTTOM;
  }
  return Ogre::GuiVerticalAlignment::GVA_TOP;
}

void copyImageToPixelBox(const QImage & source_image, const Ogre::PixelBox & pixel_box)
{
  auto * destination = static_cast<unsigned char *>(pixel_box.data);
  const auto destination_row_step =
    static_cast<std::size_t>(pixel_box.rowPitch) * sizeof(std::uint32_t);
  const auto row_bytes = static_cast<std::size_t>(source_image.width()) * sizeof(std::uint32_t);

  for (int row = 0; row < source_image.height(); ++row) {
    std::memcpy(
      destination + static_cast<std::size_t>(row) * destination_row_step,
      source_image.constScanLine(row),
      row_bytes);
  }
}

class ScopedPixelBufferLock
{
public:
  explicit ScopedPixelBufferLock(Ogre::HardwarePixelBufferSharedPtr pixel_buffer)
  : pixel_buffer_(std::move(pixel_buffer))
  {
    if (pixel_buffer_) {
      pixel_buffer_->lock(Ogre::HardwareBuffer::HBL_DISCARD);
    }
  }

  ~ScopedPixelBufferLock()
  {
    if (pixel_buffer_) {
      pixel_buffer_->unlock();
    }
  }

  ScopedPixelBufferLock(const ScopedPixelBufferLock &) = delete;
  ScopedPixelBufferLock & operator=(const ScopedPixelBufferLock &) = delete;

  const Ogre::PixelBox & currentLock() const
  {
    return pixel_buffer_->getCurrentLock();
  }

private:
  Ogre::HardwarePixelBufferSharedPtr pixel_buffer_;
};

class OgreOverlayBackend final : public OverlayBackend
{
public:
  explicit OgreOverlayBackend(std::string name)
  : name_(std::move(name))
  {
  }

  ~OgreOverlayBackend() override
  {
    destroyResources();
  }

  OverlayBackendResult initialize(Ogre::SceneManager * scene_manager) override
  {
    if (!scene_manager) {
      return errorResult("RViz scene manager is unavailable");
    }

    try {
      rviz_rendering::RenderSystem::get()->prepareOverlays(scene_manager);

      auto * overlay_manager = Ogre::OverlayManager::getSingletonPtr();
      if (!overlay_manager) {
        return errorResult("Ogre overlay manager is unavailable");
      }

      overlay_ = overlay_manager->create(name_);
      panel_ = static_cast<Ogre::PanelOverlayElement *>(
        overlay_manager->createOverlayElement("Panel", name_ + "Panel"));
      panel_->setMetricsMode(Ogre::GMM_PIXELS);

      material_ = Ogre::MaterialManager::getSingleton().create(
        name_ + "Material",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
      panel_->setMaterialName(material_->getName());
      overlay_->add2D(panel_);
      overlay_->hide();
    } catch (const std::exception & exception) {
      destroyResources();
      return errorResult(exception.what());
    }

    return okResult();
  }

  OverlayBackendResult setGeometry(const OverlayGeometry & geometry) override
  {
    if (!panel_) {
      return errorResult("Overlay panel is unavailable");
    }

    geometry_ = geometry;
    const OverlaySize size = clampedOverlaySize(geometry.width, geometry.height);
    const bool clamped = geometry.width < 1 || geometry.height < 1;

    try {
      const auto position = alignedOverlayPosition(geometry);
      panel_->setDimensions(size.width, size.height);
      panel_->setHorizontalAlignment(toOgreHorizontalAlignment(geometry.horizontal_alignment));
      panel_->setVerticalAlignment(toOgreVerticalAlignment(geometry.vertical_alignment));
      panel_->setPosition(position.left, position.top);
      const OverlayBackendResult texture_result = updateTextureSize(size);
      if (!texture_result.ok()) {
        return texture_result;
      }
    } catch (const std::exception & exception) {
      return errorResult(exception.what());
    }

    if (clamped && !reported_clamped_size_) {
      reported_clamped_size_ = true;
      return warningResult("Overlay dimensions were clamped to at least one pixel");
    }
    return okResult();
  }

  void setVisible(const bool visible) override
  {
    if (!overlay_) {
      return;
    }
    if (visible) {
      overlay_->show();
    } else {
      overlay_->hide();
    }
  }

  bool isReady() const override
  {
    return overlay_ && panel_ && material_ && texture_;
  }

  OverlayBackendResult updateImage(const QImage & image) override
  {
    if (!panel_ || !material_) {
      return errorResult("Overlay is not initialized");
    }

    OverlayGeometry image_geometry = geometry_;
    image_geometry.width = image.width();
    image_geometry.height = image.height();
    const OverlaySize size = clampedOverlaySize(image_geometry.width, image_geometry.height);

    try {
      const OverlayBackendResult texture_result = updateTextureSize(size);
      if (!texture_result.ok()) {
        return texture_result;
      }

      QImage normalized = image.convertToFormat(QImage::Format_ARGB32);
      if (normalized.width() != static_cast<int>(size.width) ||
        normalized.height() != static_cast<int>(size.height))
      {
        normalized = normalized.scaled(
          static_cast<int>(size.width),
          static_cast<int>(size.height),
          Qt::IgnoreAspectRatio,
          Qt::FastTransformation);
      }

      ScopedPixelBufferLock lock(texture_->getBuffer());
      copyImageToPixelBox(normalized, lock.currentLock());
    } catch (const std::exception & exception) {
      return errorResult(exception.what());
    }

    return okResult();
  }

private:
  OverlayBackendResult updateTextureSize(const OverlaySize size)
  {
    if (!material_) {
      return errorResult("Overlay material is unavailable");
    }

    const std::string texture_name = name_ + "Texture";
    if (texture_ && texture_->getWidth() == size.width && texture_->getHeight() == size.height) {
      return okResult();
    }

    try {
      auto * texture_manager = Ogre::TextureManager::getSingletonPtr();
      if (!texture_manager) {
        return errorResult("Ogre texture manager is unavailable");
      }

      if (texture_) {
        texture_manager->remove(texture_name);
        texture_.reset();
        material_->getTechnique(0)->getPass(0)->removeAllTextureUnitStates();
      }

      texture_ = texture_manager->createManual(
        texture_name,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::TEX_TYPE_2D,
        size.width,
        size.height,
        0,
        Ogre::PF_A8R8G8B8,
        Ogre::TU_DEFAULT);

      Ogre::Pass * pass = material_->getTechnique(0)->getPass(0);
      pass->createTextureUnitState(texture_name);
      pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
    } catch (const std::exception & exception) {
      return errorResult(exception.what());
    }

    return okResult();
  }

  void destroyResources()
  {
    if (overlay_) {
      overlay_->hide();
    }

    if (texture_) {
      Ogre::TextureManager::getSingleton().remove(texture_->getName());
      texture_.reset();
    }

    auto * overlay_manager = Ogre::OverlayManager::getSingletonPtr();
    if (overlay_manager) {
      if (panel_) {
        overlay_manager->destroyOverlayElement(panel_);
        panel_ = nullptr;
      }
      if (overlay_) {
        overlay_manager->destroy(overlay_);
        overlay_ = nullptr;
      }
    }

    if (material_) {
      material_->unload();
      Ogre::MaterialManager::getSingleton().remove(material_->getName());
      material_.reset();
    }
  }

  std::string name_;
  OverlayGeometry geometry_;
  Ogre::Overlay * overlay_{nullptr};
  Ogre::PanelOverlayElement * panel_{nullptr};
  Ogre::MaterialPtr material_;
  Ogre::TexturePtr texture_;
  bool reported_clamped_size_{false};
};

}  // namespace

OverlaySize clampedOverlaySize(const int width, const int height)
{
  return OverlaySize{
    static_cast<unsigned int>(std::max(width, 1)),
    static_cast<unsigned int>(std::max(height, 1))};
}

OverlayPosition alignedOverlayPosition(const OverlayGeometry & geometry)
{
  const OverlaySize size = clampedOverlaySize(geometry.width, geometry.height);

  OverlayPosition position;
  switch (geometry.horizontal_alignment) {
    case OverlayHorizontalAlignment::Left:
      position.left = geometry.x_offset;
      break;
    case OverlayHorizontalAlignment::Center:
      position.left = geometry.x_offset - static_cast<double>(size.width) / 2.0;
      break;
    case OverlayHorizontalAlignment::Right:
      position.left = -geometry.x_offset - static_cast<double>(size.width);
      break;
  }

  switch (geometry.vertical_alignment) {
    case OverlayVerticalAlignment::Top:
      position.top = geometry.y_offset;
      break;
    case OverlayVerticalAlignment::Center:
      position.top = geometry.y_offset - static_cast<double>(size.height) / 2.0;
      break;
    case OverlayVerticalAlignment::Bottom:
      position.top = -geometry.y_offset - static_cast<double>(size.height);
      break;
  }

  return position;
}

std::unique_ptr<OverlayBackend> makeOgreOverlayBackend(std::string name)
{
  return std::make_unique<OgreOverlayBackend>(std::move(name));
}

}  // namespace rviz_2d_plot_plugin
