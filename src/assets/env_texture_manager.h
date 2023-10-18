#pragma once

// global
#include <memory>
// local
#include "assets/texture.h"
#include "id_generator.h"
#include "mi_types.h"
// fwd
namespace ui {
class WinResources;
}

class EnvTextureManager {
 public:
  EnvTextureManager(ui::WinResources &ui);
  using Inject = EnvTextureManager(ui::WinResources &);

 public:
  std::shared_ptr<EnvTexture> CreateEnvTexture(const std::string &name);
  std::shared_ptr<EnvTexture> GetEnvTexture(const std::string &name) const;
  bool IsEnvLoaded(const std::string &name) const;
  void RemoveEnvTexture(id::EnvTexture id);

 private:
  ui::WinResources &ui_;

  MiUnMap<id::EnvTexture, std::string> env_id_to_name_;
  MiUnMap<std::string, std::shared_ptr<EnvTexture>> env_map_;

  void AddEnvTexture(const std::string &path,
                     const std::shared_ptr<EnvTexture> &texture);
};