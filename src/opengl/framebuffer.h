#pragma once

// deps
#include <glad/glad.h>

#include <glm/vec2.hpp>
// global
#include <string>
// local
#include "mi_types.h"

namespace gl {
class Texture;

class RenderBuffer {
 public:
  RenderBuffer();
  ~RenderBuffer();
  // OpenGL resources (move-only)
  RenderBuffer(const RenderBuffer &) = delete;
  RenderBuffer &operator=(const RenderBuffer &) = delete;
  RenderBuffer(RenderBuffer &&o) noexcept;
  RenderBuffer &operator=(RenderBuffer &&o) noexcept;

 public:
  operator GLuint() const { return rbo_; }
  void SetStorage(GLenum internal_format, GLsizei width, GLsizei height) const;
  void SetStorage(GLenum internal_format, glm::ivec2 size) const;

 protected:
  GLuint rbo_{0};
};

class FrameBuffer {
 public:
  FrameBuffer(const std::string &name);
  ~FrameBuffer();
  // OpenGL resources (move-only)
  FrameBuffer(const FrameBuffer &) = delete;
  FrameBuffer &operator=(const FrameBuffer &) = delete;
  FrameBuffer(FrameBuffer &&o) noexcept;
  FrameBuffer &operator=(FrameBuffer &&o) noexcept;

 public:
  operator GLuint() const { return fbo_; }
  void CheckStatus() const;
  void Bind() const;
  void Clear(GLenum buffer, GLint drawbuffer, const GLfloat *value) const;
  void Clear(GLenum buffer, GLint drawbuffer, const GLuint *value) const;
  void SetDrawBuffers(std::initializer_list<GLenum> buffers) const;
  void SetDrawBuffers(const MiVector<GLenum> &buffers) const;
  void SetReadBuffer(GLenum buf) const;
  void SetDrawBuffer(GLenum buf) const;
  void AttachTexture(GLenum attachment, const gl::Texture &texture,
                     GLint level = 0) const;
  void AttachTextureLayer(GLenum attachment, const gl::Texture &texture,
                          GLint level, GLint layer) const;
  void AttachRenderBuffer(GLenum attachment,
                          const RenderBuffer &renderbuffer) const;

 protected:
  GLuint fbo_{0};
  std::string name_;
};

}  // namespace gl