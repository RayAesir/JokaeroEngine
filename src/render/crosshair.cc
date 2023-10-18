#include "crosshair.h"

// deps
#include <glm/gtc/matrix_transform.hpp>
// global
#include <numbers>
// local
#include "app/parameters.h"
#include "options.h"

Crosshair::Crosshair() {
  crosshair_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                        ShaderStorageBinding::kCrosshair);
  crosshair_.SetStorage();
}

void Crosshair::DrawCrosshair() const {
  if (opt::crosshair.flags) {
    glDrawArrays(GL_LINES, 0, index_);
  }
}

void Crosshair::UpdateCrosshair() {
  // other
  upload_crosshair_.resolution = glm::vec2(app::opengl.framebuffer_size);
  upload_crosshair_.thickness = opt::crosshair.thickness;
  upload_crosshair_.color = opt::crosshair.color;
  // mvp
  float aspect_ratio = app::opengl.aspect_ratio;
  glm::mat4 view{1.0f};
  view = glm::scale(view, glm::vec3(opt::crosshair.size));
  glm::mat4 proj =
      glm::ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);
  upload_crosshair_.proj_view = proj * view;
  // flags
  index_ = 0;
  auto& vertex = upload_crosshair_.vertex;
  if (opt::crosshair.flags & opt::CrosshairFlags::kSquare) {
    vertex[0] = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    vertex[1] = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
    vertex[2] = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
    vertex[3] = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    vertex[4] = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    vertex[5] = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
    vertex[6] = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
    vertex[7] = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    index_ += 8;
  }
  if (opt::crosshair.flags & opt::CrosshairFlags::kCross) {
    // vertical
    vertex[index_ + 0] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
    vertex[index_ + 1] =
        glm::vec4(0.0f, -opt::crosshair.inner_size, 0.0f, 1.0f);
    vertex[index_ + 2] = glm::vec4(0.0f, opt::crosshair.inner_size, 0.0f, 1.0f);
    vertex[index_ + 3] = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    // horizontal
    vertex[index_ + 4] = glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f);
    vertex[index_ + 5] =
        glm::vec4(-opt::crosshair.inner_size, 0.0f, 0.0f, 1.0f);
    vertex[index_ + 6] = glm::vec4(opt::crosshair.inner_size, 0.0f, 0.0f, 1.0f);
    vertex[index_ + 7] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    index_ += 8;
  }
  constexpr float kRadius = 0.5f;
  constexpr float kNumSides = 16;
  constexpr float kTwoPi = glm::two_pi<float>();
  constexpr float kStep = kTwoPi / kNumSides;
  // total = kNumSides * 2
  if (opt::crosshair.flags & opt::CrosshairFlags::kCircle) {
    for (float i = 0.0f; i < kTwoPi; i += kStep) {
      float x1 = kRadius * glm::cos(i);
      float y1 = kRadius * glm::sin(i);
      float x2 = kRadius * glm::cos(i + kStep);
      float y2 = kRadius * glm::sin(i + kStep);
      vertex[index_ + 0] = glm::vec4(x1, y1, 0.0f, 1.0f);
      vertex[index_ + 1] = glm::vec4(x2, y2, 0.0f, 1.0f);
      index_ += 2;
    }
  }
  // upload
  glNamedBufferSubData(crosshair_, 0, sizeof(gpu::StorageCrosshair),
                       &upload_crosshair_);
}
