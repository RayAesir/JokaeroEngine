#pragma once

// deps
#include <glm/vec2.hpp>
// global
#include <array>
#include <string>
// local
#include "ui/base_types.h"
// fwd
namespace ini {
struct Description;
}

// the system parameters
namespace app {

namespace types {

struct Glfw {
  Glfw();
  // monitor and video mode
  std::string monitor_name;
  glm::vec2 window_scale;
  glm::ivec2 videomode_size;
  int refresh_rate;
  // window
  std::string title;
  bool fullscreen;
  int cursor_state;
  bool cursor_entered;
  // framebuffer
  bool vsync;
  glm::ivec2 window_size;
  int samples;
  // time based variables
  size_t frame_count;
  float time;
  float delta_time;
  float sin_half_speed;
  float sin_full_speed;
};

struct OpenGl {
  OpenGl();

  struct Mode {
    int mode;
  };
  ui::Selectable<Mode> window_mode;

  struct Resolution {
    glm::ivec2 res;
  };
  ui::Selectable<Resolution> resolution;

  glm::ivec2 framebuffer_size;
  glm::vec2 inv_framebuffer_size;
  float aspect_ratio;
  bool resizeable;

  bool show_debug;

  struct DebugMsgSeverity {
    const char* label;
    bool enable;
    int severity;
  };
  std::array<DebugMsgSeverity, 5> msg_severity;
};

struct Imgui {
  bool ui_active{false};
  bool want_text_input{false};
  bool cursor_over_ui{false};
};

struct Gpu {
  const char* vendor;
  const char* renderer;
  int max_ssbo_bindings{0};
  int max_ubo_bindings{0};
  int max_ubo_block_size{0};
  int ubo_offset_alignment{0};
  int max_workgroup_count[3]{};
  int max_workgroup_size[3]{};
  int max_workgroup_invocations{0};
  int max_compute_shared_memory_size{0};
  int max_color_attachments{0};
  int max_drawbuffers{0};
  int max_texture_size{0};
  int max_texture_units{0};
  int subgroup_size{0};
};

struct Cpu {
  unsigned int threads_total{1};
  unsigned int task_threads{1};
};

}  // namespace types

// rw protected access
namespace hidden {

inline types::Glfw gGlfw;
inline types::OpenGl gOpengl;
inline types::Imgui gImgui;
inline types::Gpu gGpu;
inline types::Cpu gCpu;

}  // namespace hidden

// emphasize that global variables is readonly
inline const types::Glfw& glfw = hidden::gGlfw;
inline const types::OpenGl& opengl = hidden::gOpengl;
inline const types::Imgui& imgui = hidden::gImgui;
inline const types::Gpu& gpu = hidden::gGpu;
inline const types::Cpu& cpu = hidden::gCpu;

namespace set {

inline types::Glfw& glfw = hidden::gGlfw;
inline types::OpenGl& opengl = hidden::gOpengl;
inline types::Imgui& imgui = hidden::gImgui;
inline types::Gpu& gpu = hidden::gGpu;
inline types::Cpu& cpu = hidden::gCpu;

void UpdateScreenSize();
ini::Description GetIniDescription();

}  // namespace set

}  // namespace app
