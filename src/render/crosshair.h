#pragma once

// local
#include "opengl/buffer_storage.h"

namespace gpu {

struct StorageCrosshair {
  glm::mat4 proj_view;
  glm::vec4 color;
  //
  glm::vec2 resolution;
  GLfloat thickness;
  GLfloat empty_pad;
  //
  glm::vec4 vertex[64];
};

}  // namespace gpu

class Crosshair {
 public:
  Crosshair();

 public:
  void UpdateCrosshair();
  void DrawCrosshair() const;

 private:
  gl::Buffer<gpu::StorageCrosshair> crosshair_{"Crosshair"};

  GLsizei index_{0};
  // stack allocated data
  gpu::StorageCrosshair upload_crosshair_;
};