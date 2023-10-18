#pragma once

// local
#include "assets/assets.h"
#include "events.h"
#include "global.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "ui/win_layout.h"

class Engine : public event::Base<Engine> {
 public:
  Engine(Assets& assets, Scene& scene, Renderer& renderer, ui::Layout& ui);
  using Inject = Engine(Assets&, Scene&, Renderer&, ui::Layout&);

 public:
  Assets& assets_;
  Scene& scene_;

  void LoadAssets();
  void LoadEnvMaps();
  void LoadIBLArchives();
  // multi-threaded state selection
  void SetState(State::Enum state);
  // called in another thread
  void Setup(const std::function<void(Engine&)>& setup);
  void Render();

 private:
  Renderer& renderer_;
  ui::Layout& ui_;

  using MemFn = void (Engine::*)();
  std::atomic<MemFn> render_state_ = &Engine::DrawLoading;

  void DrawLoading();
  void DrawScene();

  void InitEvents();
};
