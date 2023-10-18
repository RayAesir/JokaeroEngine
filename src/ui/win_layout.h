#pragma once

// local
#include "events.h"
#include "ui/win_guizmo.h"
#include "ui/win_resources.h"
#include "ui/win_scene.h"
#include "ui/win_settings.h"
// fwd
class Assets;
class Scene;
class Renderer;

namespace ui {

struct Windows {
  bool show_settings{true};
  bool show_scene{true};
  bool show_resources{true};
  bool show_demo{false};

  bool show_look_at{true};
  bool show_context_object{false};
  bool show_guizmo{false};
};

class Layout : public event::Base<Layout> {
 public:
  Layout(WinSettings& win_settings,    //
         WinScene& win_scene,          //
         WinResources& win_resources,  //
         WinGuizmo& win_guizmo         //
  );
  using Inject = Layout(WinSettings&, WinScene&, WinResources&, WinGuizmo&);

  void BindScene(Scene* scene) { scene_ = scene; }

 public:
  WinSettings& win_settings_;
  WinScene& win_scene_;
  WinResources& win_resources_;
  WinGuizmo& win_guizmo_;

  void StateLoading();
  void StateEngine();

 private:
  Windows windows_;

  void MainMenuBar();

  void WindowLoading();

  void WindowSettings();
  void WindowScene();
  void WindowResources();
  void WindowDemo();

  void WindowLookAt();
  void WindowContextObject();
  void WindowGuizmo();

 private:
  Scene* scene_{nullptr};

  void InitEvents();
};

}  // namespace ui