#pragma once

// local
#include "global.h"
// fwd
class Renderer;

namespace ui {

struct CascadeShadowMapping {
  glm::vec3 center[global::kMaxShadowCascades]{};
  float radius[global::kMaxShadowCascades]{};
  float split[global::kMaxShadowCascades]{};
  unsigned int dop_total[global::kMaxShadowCascades]{};
  glm::vec4 dop_planes[global::kMaxShadowCascades][global::kDopPlanes]{};
};

struct DebugReadback {
  GLint* int_ptr;
  GLuint* uint_ptr;
  GLfloat* float_ptr;
  glm::vec4* vec4_ptr;
};

class WinSettings {
 public:
  WinSettings() = default;
  using Inject = WinSettings();

  void BindRenderer(Renderer* renderer) { renderer_ = renderer; }

  CascadeShadowMapping csm_;
  DebugReadback debug_readback_;

  void ShowOpenGL();
  void ShowPipeline();
  void ShowLighting();
  void ShowShadows();
  void ShowAmbientOcclusion();
  void ShowPostprocess();
  void ShowOutline();
  void ShowCrosshair();
  void ShowLayers();
  void ShowMetrics();
  void ShowDebug();
  void ShowDebugBuffer();
  void ShowMemoryInfo();

  void MenuBar();
  void Headers();

 private:
  Renderer* renderer_{nullptr};
};

}  // namespace ui