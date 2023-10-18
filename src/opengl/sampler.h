#pragma once

// deps
#include <glad/glad.h>

#include <glm/vec4.hpp>

namespace gl {

class Sampler {
 public:
  Sampler();
  ~Sampler();
  // OpenGL resources (move-only)
  Sampler(const Sampler &) = delete;
  Sampler &operator=(const Sampler &) = delete;
  Sampler(Sampler &&o) noexcept;
  Sampler &operator=(Sampler &&o) noexcept;

 public:
  operator GLuint() const { return sampler_; }
  void Bind(GLuint unit) const;
  void SetFilter(GLenum min_filter = GL_NEAREST_MIPMAP_LINEAR,
                 GLenum mag_filter = GL_LINEAR) const;
  void SetWrap2D(GLenum wrap = GL_REPEAT) const;
  void SetWrap3D(GLenum wrap = GL_REPEAT) const;
  // float, OpenGL 4.6 (GL_LINEAR_MIPMAP_LINEAR)
  void SetAnisotropy(GLfloat value) const;
  // positive - blur, negative - sharp
  void SetLoadBias(GLfloat value) const;
  void SetBorderColor(const glm::vec4 &color) const;
  // internal format should be GL_DEPTH_COMPONENT
  void EnableCompareMode(GLenum compare_func) const;
  void DisableCompareMode() const;

 protected:
  GLuint sampler_{0};
};

// presets
class Sampler2D : public Sampler {
 public:
  Sampler2D() = default;
};

class Sampler3D : public Sampler {
 public:
  Sampler3D() = default;
};

}  // namespace gl