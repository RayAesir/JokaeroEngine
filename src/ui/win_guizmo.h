#pragma once

// fwd
class Scene;
class MainCamera;

namespace ui {

struct GuizmoType {
  enum Enum : int { kObject, kPointLight, kFxInstance };
};

class WinGuizmo {
 public:
  WinGuizmo() = default;
  using Inject = WinGuizmo();

  void BindScene(Scene* scene) { scene_ = scene; }
  void BindCamera(MainCamera* camera) { camera_ = camera; }

 public:
  bool Translation(float* translation);
  bool Full(float* translation, float* euler_deg, float* scale);

  void ShowGuizmo();

 private:
  Scene* scene_{nullptr};
  MainCamera* camera_{nullptr};

  int guizmo_type_;
};

}  // namespace ui