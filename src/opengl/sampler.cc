#include "sampler.h"

// global
#include <utility>

namespace gl {

Sampler::Sampler() { glCreateSamplers(1, &sampler_); }

Sampler::~Sampler() { glDeleteSamplers(1, &sampler_); }

Sampler::Sampler(Sampler &&o) noexcept
    : sampler_(std::exchange(o.sampler_, 0)){};

Sampler &Sampler::operator=(Sampler &&o) noexcept {
  if (&o == this) return *this;
  if (sampler_) glDeleteSamplers(1, &sampler_);

  sampler_ = std::exchange(o.sampler_, 0);
  return *this;
}

void Sampler::Bind(GLuint unit) const { glBindSampler(unit, sampler_); }

void Sampler::SetFilter(GLenum min_filter, GLenum mag_filter) const {
  glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, min_filter);
  glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void Sampler::SetWrap2D(GLenum wrap) const {
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, wrap);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, wrap);
}

void Sampler::SetWrap3D(GLenum wrap) const {
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, wrap);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, wrap);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_R, wrap);
}

void Sampler::SetAnisotropy(GLfloat value) const {
  glSamplerParameterf(sampler_, GL_TEXTURE_MAX_ANISOTROPY, value);
}

void Sampler::SetLoadBias(GLfloat value) const {
  glSamplerParameterf(sampler_, GL_TEXTURE_LOD_BIAS, value);
}

void Sampler::SetBorderColor(const glm::vec4 &color) const {
  glSamplerParameterfv(sampler_, GL_TEXTURE_BORDER_COLOR, &color[0]);
}

void Sampler::EnableCompareMode(GLenum compare_func) const {
  glSamplerParameteri(sampler_, GL_TEXTURE_COMPARE_MODE,
                      GL_COMPARE_REF_TO_TEXTURE);
  glSamplerParameteri(sampler_, GL_TEXTURE_COMPARE_FUNC, compare_func);
}

void Sampler::DisableCompareMode() const {
  glSamplerParameteri(sampler_, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

}  // namespace gl