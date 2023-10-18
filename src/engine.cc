#include "engine.h"

// local
#include "app/application.h"
#include "app/input.h"
#include "app/main_thread.h"
#include "app/parameters.h"
#include "app/task_system.h"
#include "files.h"

Engine::Engine(Assets &assets, Scene &scene, Renderer &renderer, ui::Layout &ui)
    : event::Base<Engine>(&Engine::InitEvents, this),
      assets_(assets),
      scene_(scene),
      renderer_(renderer),
      ui_(ui) {
  ui_.BindScene(&scene_);

  ui_.win_settings_.BindRenderer(&renderer_);

  ui_.win_scene_.BindAssets(&assets_);
  ui_.win_scene_.BindScene(&scene_);
  ui_.win_scene_.BindCamera(&scene_.camera_);
  ui_.win_scene_.BindRenderer(&renderer_);

  ui_.win_resources_.BindAssets(&assets_);
  ui_.win_resources_.BindScene(&scene_);

  ui_.win_guizmo_.BindScene(&scene_);
  ui_.win_guizmo_.BindCamera(&scene_.camera_);
}

void Engine::InitEvents() {
  Connect(Event::kCloseWindow, []() { app::CloseWindow(); });
  Connect(Event::kToggleInputMode, []() {
    app::set::imgui.ui_active = !app::imgui.ui_active;
    app::ToggleInputMode(app::imgui.ui_active);
  });
  Connect(Event::kSetResizeableWindow,
          []() { app::SetResizeableWindow(app::opengl.resizeable); });
  Connect(Event::kSetWindowMode,
          []() { app::SetWindowMode(app::opengl.window_mode.current); });
  Connect(Event::kToggleOpenGlDebug,
          []() { app::ToggleOpenGlDebug(app::opengl.show_debug); });
  Connect(Event::kApplyOpenGlMessageSeverity,
          []() { app::ApplyOpenGlMessageSeverity(); });
}

void Engine::LoadAssets() {
  auto paths = files::meshes.GetFilePaths();

  for (const auto &path : paths) {
    app::task::PushTask([this, &path](unsigned int thread_id) {
      assets_.models_.LoadModelMt(path, thread_id);
    });
  }
  app::task::WaitForTasks();

  // batch upload misc parameters (optimization)
  app::task::PushTask([this](unsigned int thread_id) {
    assets_.models_.UploadCmdBoxMaterial();
  });
  app::task::WaitForTasks();
}

void Engine::LoadEnvMaps() {
  auto paths = files::env_maps.GetFilePaths();

  std::packaged_task<void()> package([this, &paths]() {
    for (const auto &path : paths) {
      Image img{path.string()};
      if (img.success == false) continue;

      Texture source{img, TextureType::kDiffuse};
      // empty env texture (allocate memory)
      auto env_tex = assets_.env_tex_.CreateEnvTexture(path.stem().string());
      renderer_.RenderEnvironmentTexture(source.tbo_, env_tex);
    }
  });

  auto future = app::main_thread::PushTask(package);
  future.get();
}

void Engine::LoadIBLArchives() {
  struct IBLArchiveInfo {
    std::string name;
    std::string skytex_path;
    std::string irradiance_path;
    std::string prefiltered_path;
  };

  MiVector<IBLArchiveInfo> ibl_archives;
  const auto &load_path = files::ibl_archives.path;
  if (fs::exists(load_path)) {
    ibl_archives.reserve(files::ibl_archives.dirs_count);

    for (const fs::directory_entry &dir_entry :
         fs::directory_iterator(load_path)) {
      IBLArchiveInfo info;
      int check = 0;
      for (const fs::directory_entry &entry :
           fs::recursive_directory_iterator(dir_entry)) {
        const auto &path = entry.path();
        std::string stem = path.stem().string();
        if (stem.ends_with("_i")) {
          info.irradiance_path = path.string();
          ++check;
        }
        if (stem.ends_with("_p")) {
          info.prefiltered_path = path.string();
          ++check;
        }
        if (stem.ends_with("_s")) {
          info.skytex_path = path.string();
          info.name = stem;
          ++check;
        }
      }

      if (check == 3) {
        ibl_archives.push_back(info);
      } else {
        spdlog::warn("{}: Incomplete IBL Archive '{}'", __FUNCTION__,
                     dir_entry.path().string());
      }
    }
  }

  std::packaged_task<void()> package([this, &ibl_archives]() {
    for (const auto &ibl : ibl_archives) {
      ImageHdr img_skytex{ibl.skytex_path};
      if (img_skytex.success == false) continue;
      //
      Image img_irradiance{ibl.irradiance_path};
      if (img_irradiance.success == false) continue;
      //
      ImageHdr img_prefiltered{ibl.prefiltered_path};
      if (img_prefiltered.success == false) continue;

      Texture skytex{img_skytex};
      Texture irradiance{img_irradiance, TextureType::kNormals};
      Texture prefiltered{img_prefiltered};
      auto env_tex = assets_.env_tex_.CreateEnvTexture(ibl.name);
      renderer_.RenderIBLArchive(skytex.tbo_, irradiance.tbo_, prefiltered.tbo_,
                                 env_tex);
    }
  });
  auto future = app::main_thread::PushTask(package);
  future.get();
}

void Engine::SetState(State::Enum state) {
  static constexpr MemFn selector[State::kTotal]{
      &Engine::DrawLoading,
      &Engine::DrawScene,
  };
  render_state_ = selector[state];
  global::gState = state;
}

void Engine::Setup(const std::function<void(Engine &)> &setup) {
  std::thread load_task(setup, std::ref(*this));
  load_task.detach();
}

void Engine::Render() {
  // all events once per frame
  event::ProcessInput();
  event::ProcessCustomEvents();
  (this->*render_state_)();
}

void Engine::DrawLoading() {
  ui_.StateLoading();
  renderer_.RenderLoading();
}

void Engine::DrawScene() {
  // input's callbacks already done before Render()
  // UI events done here
  ui_.StateEngine();

  static bool call_once = [this]() {
    // after loading screen
    input::ResetMouse();

    // check Scene for env map
    if (scene_.env_tex_ == nullptr) {
      scene_.env_tex_ = assets_.env_tex_.GetEnvTexture("Default Env Map");
      spdlog::error("{}: Environment textures not loaded", __FUNCTION__);
    }

    // for IBL
    renderer_.GenerateBrdfLut();
    return true;
  }();

  renderer_.RenderScene();
}
