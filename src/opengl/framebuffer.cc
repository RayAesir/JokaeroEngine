#include "framebuffer.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "mem_info.h"
#include "opengl/texture.h"

namespace gl {

RenderBuffer::RenderBuffer() { glCreateRenderbuffers(1, &rbo_); }

RenderBuffer::~RenderBuffer() {
  glDeleteRenderbuffers(1, &rbo_);
  mem::Erase(mem::kTexture, this);
}

RenderBuffer::RenderBuffer(RenderBuffer &&o) noexcept
    : rbo_(std::exchange(o.rbo_, 0)) {
  mem::Move(mem::kTexture, &o, this);
};

RenderBuffer &RenderBuffer::operator=(RenderBuffer &&o) noexcept {
  if (&o == this) return *this;
  if (rbo_) glDeleteRenderbuffers(1, &rbo_);

  rbo_ = std::exchange(o.rbo_, 0);
  mem::Move(mem::kTexture, &o, this);
  return *this;
}

void RenderBuffer::SetStorage(GLenum internal_format, GLsizei width,
                              GLsizei height) const {
  glNamedRenderbufferStorage(rbo_, internal_format, width, height);
  auto bytes = mem::CalcTexBytes2D(internal_format, 1, width * height);
  mem::Add(mem::kTexture, this, bytes);
}

void RenderBuffer::SetStorage(GLenum internal_format, glm::ivec2 size) const {
  glNamedRenderbufferStorage(rbo_, internal_format, size.x, size.y);
  auto bytes = mem::CalcTexBytes2D(internal_format, 1, size.x * size.y);
  mem::Add(mem::kTexture, this, bytes);
}

FrameBuffer::FrameBuffer(const std::string &name) : name_(name) {
  glCreateFramebuffers(1, &fbo_);
}

FrameBuffer::~FrameBuffer() { glDeleteFramebuffers(1, &fbo_); }

FrameBuffer::FrameBuffer(FrameBuffer &&o) noexcept
    : fbo_(std::exchange(o.fbo_, 0)), name_(std::move(o.name_)){};

FrameBuffer &FrameBuffer::operator=(FrameBuffer &&o) noexcept {
  if (&o == this) return *this;
  if (fbo_) glDeleteFramebuffers(1, &fbo_);

  fbo_ = std::exchange(o.fbo_, 0);
  name_ = std::move(o.name_);
  return *this;
}

void FrameBuffer::CheckStatus() const {
  GLenum status = glCheckNamedFramebufferStatus(fbo_, GL_FRAMEBUFFER);
  switch (status) {
    case GL_FRAMEBUFFER_UNDEFINED:
      spdlog::error("GL_FRAMEBUFFER_UNDEFINED '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER '{}'", name_);
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      spdlog::error("GL_FRAMEBUFFER_UNSUPPORTED '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE '{}'", name_);
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
      spdlog::error("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS '{}'", name_);
      break;
    case GL_FRAMEBUFFER_COMPLETE:
      spdlog::info("GL_FRAMEBUFFER_COMPLETE '{}'", name_);
      break;
    default:
      spdlog::warn("{}: UNKOWN_STATUS '{}'", __FUNCTION__, name_);
      break;
  }
}

void FrameBuffer::Bind() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo_); }

void FrameBuffer::Clear(GLenum buffer, GLint drawbuffer,
                        const GLfloat *value) const {
  glClearNamedFramebufferfv(fbo_, buffer, drawbuffer, value);
}

void FrameBuffer::Clear(GLenum buffer, GLint drawbuffer,
                        const GLuint *value) const {
  glClearNamedFramebufferuiv(fbo_, buffer, drawbuffer, value);
}

void FrameBuffer::SetDrawBuffers(std::initializer_list<GLenum> buffers) const {
  GLsizei n = static_cast<GLsizei>(buffers.size());
  const GLenum *bufs = buffers.begin();
  glNamedFramebufferDrawBuffers(fbo_, n, bufs);
}

void FrameBuffer::SetDrawBuffers(const MiVector<GLenum> &buffers) const {
  GLsizei n = static_cast<GLsizei>(buffers.size());
  const GLenum *bufs = buffers.data();
  glNamedFramebufferDrawBuffers(fbo_, n, bufs);
}

void FrameBuffer::SetReadBuffer(GLenum buf) const {
  glNamedFramebufferReadBuffer(fbo_, buf);
}

void FrameBuffer::SetDrawBuffer(GLenum buf) const {
  glNamedFramebufferDrawBuffer(fbo_, buf);
}

void FrameBuffer::AttachTexture(GLenum attachment, const gl::Texture &texture,
                                GLint level) const {
  glNamedFramebufferTexture(fbo_, attachment, texture, level);
}

void FrameBuffer::AttachTextureLayer(GLenum attachment,
                                     const gl::Texture &texture, GLint level,
                                     GLint layer) const {
  glNamedFramebufferTextureLayer(fbo_, attachment, texture, level, layer);
}

void FrameBuffer::AttachRenderBuffer(GLenum attachment,
                                     const RenderBuffer &renderbuffer) const {
  glNamedFramebufferRenderbuffer(fbo_, attachment, GL_RENDERBUFFER,
                                 renderbuffer);
}

}  // namespace gl