#pragma once

// fwd
class Assets;
class Scene;
class Renderer;
class MainCamera;
class Model;
class Object;
class AnimatedObject;
class PointLight;
struct FxInstance;

namespace ui {

class WinScene {
 public:
  WinScene() = default;
  using Inject = WinScene();

  void BindAssets(Assets* assets) { assets_ = assets; }
  void BindScene(Scene* scene) { scene_ = scene; }
  void BindCamera(MainCamera* camera) { camera_ = camera; }
  void BindRenderer(Renderer* renderer) { renderer_ = renderer; }

 public:
  void ShowScene();
  void ShowActiveCamera(MainCamera& camera);
  void ShowActivePointLight(PointLight& light);
  void ShowActiveObject(Model& model, Object& object);
  void ShowActiveObjectMaterials(Model& model);
  void ShowActiveObjectAnimations(Model& model, AnimatedObject& object);
  void ShowContextObject(Model& model, Object& object);
  void ShowFxInstance(FxInstance& fx_instance);

  void Headers();

 private:
  Assets* assets_{nullptr};
  Scene* scene_{nullptr};
  MainCamera* camera_{nullptr};
  Renderer* renderer_{nullptr};
};

}  // namespace ui
