#pragma once

// local
#include "id_generator.h"
#include "ui/base_types.h"
// fwd
class Assets;
class Scene;
class EnvTextureManager;
struct FxPreset;

namespace ui {

struct LoadingInfo {
  MiVector<const char*> models;
};

struct CameraRow {
  id::Camera id;
  const char* name;
};

struct ModelRow {
  id::Model id;
  const char* name;
  unsigned int objects_count;
};

struct TextureRow {
  id::Texture id;
  unsigned int tbo;
  const char* path;
  const char* type;
  long count;
};

struct TextureEnvRow {
  id::EnvTexture id;
  const char* path;
};

struct Fx {
  FxPreset* fx{nullptr};
};

class WinResources {
 public:
  WinResources() = default;
  using Inject = WinResources();

  void BindAssets(Assets* assets) { assets_ = assets; }
  void BindScene(Scene* scene) { scene_ = scene; }

 public:
  LoadingInfo loading_info_;

  Table<CameraRow> table_camera_;
  Table<ModelRow> table_model_;
  Table<TextureRow> table_texture_;
  Table<TextureEnvRow> table_texture_env_;
  Selectable<Fx> list_fx_;

  void ShowLoadingInfo();

  void ShowTableCamera();
  void ShowTableModel();
  void ShowTableTexture();
  void ShowTableTextureEnv();
  void ShowFxPresets();

  void MenuBar();
  void TabBar();

 private:
  Assets* assets_{nullptr};
  Scene* scene_{nullptr};

  void RequestTextureCount();
  void RequestObjectsCount();
};

}  // namespace ui