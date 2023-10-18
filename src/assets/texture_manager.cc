#include "texture_manager.h"

// deps
#include <spdlog/spdlog.h>
#include <stb_image.h>
// local
#include "app/main_thread.h"
#include "files.h"
#include "options.h"
#include "ui/win_resources.h"
#include "utils/enums.h"

TextureManager::TextureManager(ui::WinResources &ui) : ui_(ui) {
  // stbi global variables (OpenGL)
  // or use Assimp's postprocess flag aiProcess_FlipUVs
  stbi_set_flip_vertically_on_load(true);

  // https://stackoverflow.com/questions/42886835/modifying-parameters-of-bindless-resident-textures
  // texture and sampler are resident (immutable)
  // create all sampler variants
  for (size_t a = 0; a < samplers_.size(); ++a) {
    for (size_t b = 0; b < samplers_.size(); ++b) {
      samplers_[a][b].SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
      samplers_[a][b].SetWrap2D(GL_REPEAT);
      samplers_[a][b].SetAnisotropy(opt::textures.anisotropy.list[a].value);
      samplers_[a][b].SetLoadBias(opt::textures.lod_bias.list[b].value);
    }
  }

  error_path_ = files::textures.str + "/error.png";
  blank_black0_path_ = files::textures.str + "/blank_black_alpha_zero.png";
  blank_black1_path_ = files::textures.str + "/blank_black_alpha_one.png";
  blank_white1_path_ = files::textures.str + "/blank_white_alpha_one.png";

  // the must have blank and error textures
  CreateInternalTexture(error_path_, TextureType::kDiffuse);
  CreateInternalTexture(blank_black0_path_, TextureType::kDiffuse);
  CreateInternalTexture(blank_black1_path_, TextureType::kDiffuse);
  CreateInternalTexture(blank_white1_path_, TextureType::kDiffuse);
}

void TextureManager::CreateInternalTexture(const std::string &path,
                                           TextureType::Enum type) {
  Image img{path};
  if (img.success) {
    auto texture = std::make_shared<SmartTexture>(img, type, *this);
    AddTextureMt(path, type, texture);
    CreateTexHandlerMainThread(*texture);
    internal_textures_.push_back(texture);
  } else {
    spdlog::error("{}: Not found '{}'", __FUNCTION__, path);
  }
}

void TextureManager::AddTextureMt(
    const std::string &path, TextureType::Enum type,
    const std::shared_ptr<SmartTexture> &texture) {
  std::scoped_lock lock(mutex_);
  auto [it, result] = tex_map_.try_emplace(path, texture);
  id_to_path_.try_emplace(texture->GetId(), path);

  ui_.table_texture_.AddRow(texture->GetId(), *texture, it->first.c_str(),
                            NamedEnum<TextureType::Enum>::ToStr()[type], 0);
}

GLuint TextureManager::GetCurrentSampler() const {
  const auto anisotropy = opt::textures.anisotropy.current;
  const auto lod_bias = opt::textures.lod_bias.current;
  return samplers_[anisotropy][lod_bias];
}

void TextureManager::MakeTexHandler(SmartTexture &texture) {
  GLuint sampler = GetCurrentSampler();
  GLuint tbo = texture;
  GLuint64 handler = glGetTextureSamplerHandleARB(tbo, sampler);
  texture.SetHandler(handler);
  glMakeTextureHandleResidentARB(handler);
}

// https://community.khronos.org/t/bindless-textures-resident-textures-limits/109122/2
// resident textures have a limit, space for the handlers is allocated by OS
// driver
void TextureManager::CreateTexHandlerMainThread(SmartTexture &texture) {
  // handlers must be resident on the main context
  if (app::main_thread::IsMainThread()) {
    MakeTexHandler(texture);
  } else {
    std::packaged_task<void()> package(
        [this, &texture]() { MakeTexHandler(texture); });
    auto future = app::main_thread::PushTask(package);
    future.get();
  }
}

std::shared_ptr<SmartTexture> TextureManager::CreateTextureMt(
    const std::string &path, TextureType::Enum type) {
  if (IsLoadedMt(path)) return GetTextureMt(path);

  Image img{path};
  if (img.success == false) return GetTextureMt(error_path_);

  sync_.BeginMt();
  auto texture = std::make_shared<SmartTexture>(img, type, *this);
  sync_.EndMt();

  AddTextureMt(path, type, texture);
  CreateTexHandlerMainThread(*texture);
  return texture;
}

std::shared_ptr<SmartTexture> TextureManager::GetTextureMt(
    const std::string &path) const {
  std::scoped_lock lock(mutex_);

  auto it = tex_map_.find(path);
  if (it == tex_map_.end()) {
    spdlog::warn("{}: Not found '{}'", __FUNCTION__, path);
    return tex_map_.find(error_path_)->second.lock();
  }
  return it->second.lock();
}

void TextureManager::RemoveTextureMt(id::Texture id, GLuint64 handler) {
  std::scoped_lock lock(mutex_);

  // use main context
  if (app::main_thread::IsMainThread()) {
    glMakeTextureHandleNonResidentARB(handler);
  } else {
    std::packaged_task<void()> package(
        [handler]() { glMakeTextureHandleNonResidentARB(handler); });
    auto future = app::main_thread::PushTask(package);
    future.get();
  }
  tex_map_.erase(id_to_path_.at(id));
  id_to_path_.erase(id);

  ui_.table_texture_.DeleteRow(&ui::TextureRow::id, id);
}

bool TextureManager::IsLoadedMt(const std::string &path) const {
  std::scoped_lock lock(mutex_);
  return tex_map_.contains(path);
}

void TextureManager::ApplyTextureSettingsMainThread() {
  for (const auto &[name, weak_ptr] : tex_map_) {
    auto &texture = *weak_ptr.lock();
    // remove old
    GLuint64 old_handler = texture.GetHandler();
    glMakeTextureHandleNonResidentARB(old_handler);
    // make new pair (same settings produce same handlers)
    MakeTexHandler(texture);
  }
}

MiUnMap<id::Texture, long> TextureManager::CalcTextureUseCount() noexcept {
  std::scoped_lock lock(mutex_);
  MiUnMap<id::Texture, long> id_to_count;
  id_to_count.reserve(tex_map_.size());
  for (const auto &[name, weak_ptr] : tex_map_) {
    auto texture = weak_ptr.lock();
    long count = texture.use_count() - 1;
    id_to_count.try_emplace(texture->GetId(), count);
  }
  return id_to_count;
}