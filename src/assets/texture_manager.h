#pragma once

// global
#include <array>
#include <memory>
#include <mutex>
// local
#include "assets/texture.h"
#include "mi_types.h"
#include "opengl/fence_sync.h"
#include "opengl/sampler.h"
// fwd
namespace ui {
class WinResources;
}

class TextureManager {
 public:
  TextureManager(ui::WinResources &ui);
  using Inject = TextureManager(ui::WinResources &);

 public:
  // profiling (i7-6700k, RTX 3070 Ti)
  // image to RAM: ~90% (stbi, png - longest time, tga - compressed best)
  // OpenGL texture: ~10% avg
  std::shared_ptr<SmartTexture> CreateTextureMt(const std::string &path,
                                                TextureType::Enum type);
  std::shared_ptr<SmartTexture> GetTextureMt(const std::string &path) const;
  bool IsLoadedMt(const std::string &path) const;
  void RemoveTextureMt(id::Texture id, GLuint64 handler);

  void ApplyTextureSettingsMainThread();
  MiUnMap<id::Texture, long> CalcTextureUseCount() noexcept;

 private:
  ui::WinResources &ui_;

  mutable std::mutex mutex_;
  gl::Sync sync_;
  std::array<std::array<gl::Sampler2D, 5>, 5> samplers_;
  // the members order is important!
  // TBO can be deleted and created with the same value, use unique id
  MiUnMap<id::Texture, std::string> id_to_path_;
  MiUnMap<std::string, std::weak_ptr<SmartTexture>> tex_map_;
  MiVector<std::shared_ptr<SmartTexture>> internal_textures_;

  std::string error_path_;
  std::string blank_black0_path_;
  std::string blank_black1_path_;
  std::string blank_white1_path_;

  GLuint GetCurrentSampler() const;
  void AddTextureMt(const std::string &path, TextureType::Enum type,
                    const std::shared_ptr<SmartTexture> &texture);

  void MakeTexHandler(SmartTexture &texture);
  void CreateTexHandlerMainThread(SmartTexture &texture);

  void CreateInternalTexture(const std::string &path, TextureType::Enum type);
};