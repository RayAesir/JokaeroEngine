#include "env_texture_manager.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "ui/win_resources.h"

EnvTextureManager::EnvTextureManager(ui::WinResources &ui) : ui_(ui) {}

std::shared_ptr<EnvTexture> EnvTextureManager::CreateEnvTexture(
    const std::string &name) {
  if (IsEnvLoaded(name)) return GetEnvTexture(name);

  auto env_tex = std::make_shared<EnvTexture>();
  AddEnvTexture(name, env_tex);
  return env_tex;
}

std::shared_ptr<EnvTexture> EnvTextureManager::GetEnvTexture(
    const std::string &name) const {
  auto it = env_map_.find(name);
  if (it == env_map_.end()) {
    spdlog::error("{}: Not found '{}'", __FUNCTION__, name);
    if (env_map_.size() == 0) return nullptr;
    // put something if exist
    return env_map_.begin()->second;
  }
  return it->second;
}

void EnvTextureManager::AddEnvTexture(
    const std::string &name, const std::shared_ptr<EnvTexture> &texture) {
  auto [it, result] = env_map_.try_emplace(name, texture);
  env_id_to_name_.try_emplace(texture->GetId(), name);

  ui_.table_texture_env_.AddRow(texture->GetId(), it->first.c_str());
}

void EnvTextureManager::RemoveEnvTexture(id::EnvTexture id) {
  env_map_.erase(env_id_to_name_.at(id));
  env_id_to_name_.erase(id);

  ui_.table_texture_env_.DeleteRow(&ui::TextureEnvRow::id, id);
}

bool EnvTextureManager::IsEnvLoaded(const std::string &name) const {
  return env_map_.contains(name);
}
