#include "win_layout.h"

// deps
#include <fmt/core.h>
#include <imgui.h>
// local
#include "app/ini.h"
#include "app/parameters.h"
#include "engine.h"
#include "events.h"
#include "ui/loading_indicator.h"
#include "ui/utility.h"

namespace ui {

Layout::Layout(WinSettings& win_settings,    //
               WinScene& win_scene,          //
               WinResources& win_resources,  //
               WinGuizmo& win_guizmo         //
               )
    : event::Base<Layout>(&Layout::InitEvents, this),
      win_settings_(win_settings),
      win_scene_(win_scene),
      win_resources_(win_resources),
      win_guizmo_(win_guizmo) {}

void Layout::InitEvents() {
  Connect(Event::kToggleSettingsWindow,
          [this]() { windows_.show_settings = !windows_.show_settings; });
  Connect(Event::kToggleActiveWindow,
          [this]() { windows_.show_scene = !windows_.show_scene; });
  Connect(Event::kToggleResourcesWindow,
          [this]() { windows_.show_resources = !windows_.show_resources; });
  Connect(Event::kToggleDemoWindow,
          [this]() { windows_.show_demo = !windows_.show_demo; });
  Connect(Event::kToggleGuizmo,
          [this]() { windows_.show_guizmo = !windows_.show_guizmo; });
  Connect(Event::kSelectedObjectContextMenu, [this]() {
    if (scene_->selected_object_) {
      windows_.show_context_object = true;
      if (app::imgui.ui_active == false) {
        event::Invoke(Event::kToggleInputMode);
      }
    }
  });
}

void Layout::MainMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      ImGui::MenuItem("Settings (1)", nullptr, &windows_.show_settings);
      ImGui::MenuItem("Scene (2)", nullptr, &windows_.show_scene);
      ImGui::MenuItem("Resources (3)", nullptr, &windows_.show_resources);
      ImGui::MenuItem("ImGui Demo (4)", nullptr, &windows_.show_demo);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("About")) {
      ImGui::Text("About text");
      ImGui::EndMenu();
    }

    if (ImGui::Button("Reload Shaders")) {
      event::Invoke(Event::kReloadShaders);
    }

    // align text to right
    static std::string framerate;
    fmt::format_to(std::back_inserter(framerate),
                   "Application average {:.3f} ms/frame ({:.1f} FPS)",
                   1000.0f / ImGui::GetIO().Framerate,
                   ImGui::GetIO().Framerate);
    const ImVec2 window_size = ImGui::GetWindowSize();
    const ImVec2 text_size = ImGui::CalcTextSize(framerate.c_str());
    ImGui::SetCursorPosX(window_size.x - text_size.x - 8.0f);
    ImGui::Text(framerate.c_str());
    framerate.clear();

    ImGui::EndMainMenuBar();
  }
}

void Layout::WindowLoading() {
  static ImGuiWindowFlags window_flags{
      ImGuiWindowFlags_NoTitleBar |    //
      ImGuiWindowFlags_NoScrollbar |   //
      ImGuiWindowFlags_NoCollapse |    //
      ImGuiWindowFlags_NoDecoration |  //
      ImGuiWindowFlags_NoBackground    //
  };
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::Begin("WinLoading", nullptr, window_flags);
  win_resources_.ShowLoadingInfo();

  static constexpr ImVec4 main_color{0.95f, 0.95f, 0.95f, 0.9f};
  static constexpr ImVec4 drop_color{0.2f, 0.2f, 0.2f, 0.0f};
  static float indicator_radius = 100.0f * app::glfw.window_scale.x;
  static float speed = 1.0f;
  static int count = 11;

  const ImVec2& center = viewport->GetCenter();
  ImGui::SetCursorPosX(center.x - (indicator_radius * 0.5f));
  ImGui::SetCursorPosY(center.y - (indicator_radius * 0.5f));

  ShowLoadingIndicatorCircle("Circle", indicator_radius, main_color, drop_color,
                             count, speed);
  ImGui::End();
}

void Layout::WindowSettings() {
  static ImGuiWindowFlags window_flags{
      ImGuiWindowFlags_NoMove |                 //
      ImGuiWindowFlags_NoCollapse |             //
      ImGuiWindowFlags_MenuBar |                //
      ImGuiWindowFlags_AlwaysVerticalScrollbar  //
  };
  SetCornerWindowPos(WinCorner::kTopLeft, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

  ImGui::Begin("WinSettings", &windows_.show_settings, window_flags);
  win_settings_.MenuBar();
  win_settings_.Headers();
  ImGui::End();
}

void Layout::WindowScene() {
  static ImGuiWindowFlags window_flags{
      ImGuiWindowFlags_NoMove |                 //
      ImGuiWindowFlags_NoCollapse |             //
      ImGuiWindowFlags_AlwaysVerticalScrollbar  //
  };
  SetCornerWindowPos(WinCorner::kTopRight, ImGuiCond_Always,
                     ImVec2(0.0f, 0.0f));

  ImGui::Begin("WinScene", &windows_.show_scene, window_flags);
  win_scene_.Headers();
  ImGui::End();
}

void Layout::WindowResources() {
  static ImGuiWindowFlags window_flags{
      ImGuiWindowFlags_NoMove |                 //
      ImGuiWindowFlags_NoCollapse |             //
      ImGuiWindowFlags_MenuBar |                //
      ImGuiWindowFlags_AlwaysVerticalScrollbar  //
  };
  SetCornerWindowPos(WinCorner::kBottomRight, ImGuiCond_Always,
                     ImVec2(0.0f, 0.0f));

  ImGui::Begin("WinResources", &windows_.show_resources, window_flags);
  win_resources_.MenuBar();
  win_resources_.TabBar();
  ImGui::End();
}

void Layout::WindowDemo() { ImGui::ShowDemoWindow(); }

void Layout::WindowLookAt() {
  static ImGuiWindowFlags flags{
      ImGuiWindowFlags_NoMove |            //
      ImGuiWindowFlags_NoDecoration |      //
      ImGuiWindowFlags_AlwaysAutoResize |  //
      ImGuiWindowFlags_NoSavedSettings |   //
      ImGuiWindowFlags_NoFocusOnAppearing  //
  };
  SetCenterWindowPos(WinCenter::kTop, ImGuiCond_Always, 16.0f);
  ImGui::SetNextWindowBgAlpha(0.35f);

  ImGui::Begin("LookAtOverlay", &windows_.show_look_at, flags);
  auto* look_at = scene_->look_at_object_;
  if (look_at) {
    const auto& model = look_at->GetModel();
    float dist = scene_->dist_to_object_;
    ImGui::Text("Look at Object: %s %llu", model.GetName().c_str(),
                look_at->GetId());
    ImGui::Text("Distance to Object: %.3f", dist);
  } else {
    ImGui::Text("Look at Object: None");
    ImGui::Text("Distance to Object: -");
  }
  ImGui::End();
}

void Layout::WindowContextObject() {
  static Delay delay{100.0f};
  static ImGuiWindowFlags window_flags{
      ImGuiWindowFlags_NoMove |            //
      ImGuiWindowFlags_NoDecoration |      //
      ImGuiWindowFlags_AlwaysAutoResize |  //
      ImGuiWindowFlags_NoSavedSettings     //
  };
  SetCenterWindowPos(WinCenter::kCenter, ImGuiCond_Appearing, 0.0f);

  ImGui::Begin("Context", &windows_.show_context_object, window_flags);

  auto& object = *scene_->selected_object_;
  win_scene_.ShowContextObject(object.GetModel(), object);

  if (ImGui::Button("Delete##ctx")) {
    scene_->DeleteSelectedObject();
    windows_.show_context_object = false;
    event::Invoke(Event::kToggleInputMode);
    delay.Done();
  }

  const bool win_hovered =
      ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
  const bool item_hovered = ImGui::IsAnyItemHovered();
  const bool right_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  // wait some time to avoid self-closing via mouse click
  delay.Wait();
  if (delay) {
    if (!win_hovered && !item_hovered && right_clicked) {
      windows_.show_context_object = false;
      event::Invoke(Event::kToggleInputMode);
      delay.Done();
    }
  }

  ImGui::End();
}

void Layout::WindowGuizmo() {
  static ImGuiWindowFlags flags{
      ImGuiWindowFlags_NoMove |            //
      ImGuiWindowFlags_NoDecoration |      //
      ImGuiWindowFlags_AlwaysAutoResize |  //
      ImGuiWindowFlags_NoSavedSettings |   //
      ImGuiWindowFlags_NoFocusOnAppearing  //
  };
  SetCenterWindowPos(WinCenter::kBottom, ImGuiCond_Always, 16.0f);
  ImGui::SetNextWindowBgAlpha(0.35f);

  ImGui::Begin("WinGuizmo", &windows_.show_guizmo, flags);
  win_guizmo_.ShowGuizmo();
  ImGui::End();
}

void Layout::StateLoading() {
  MainMenuBar();
  WindowLoading();
}

void Layout::StateEngine() {
  MainMenuBar();

  if (windows_.show_settings) WindowSettings();
  if (windows_.show_scene) WindowScene();
  if (windows_.show_resources) WindowResources();
  if (windows_.show_demo) WindowDemo();

  if (windows_.show_look_at) WindowLookAt();
  // Object selection with mouse cursor, when context window open
  // check pointer
  if (windows_.show_context_object && scene_->selected_object_) {
    WindowContextObject();
  }
  if (windows_.show_guizmo) WindowGuizmo();
}

}  // namespace ui