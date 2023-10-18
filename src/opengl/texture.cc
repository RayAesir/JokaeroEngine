#include "texture.h"

// global
#include <utility>
// local
#include "mem_info.h"

namespace gl {

Texture::Texture(GLenum target) { glCreateTextures(target, 1, &tbo_); }

Texture::~Texture() {
  glDeleteTextures(1, &tbo_);
  mem::Erase(mem::kTexture, this);
}

Texture::Texture(Texture &&o) noexcept : tbo_(std::exchange(o.tbo_, 0)) {
  mem::Move(mem::kTexture, &o, this);
};

Texture &Texture::operator=(Texture &&o) noexcept {
  if (&o == this) return *this;
  if (tbo_) glDeleteTextures(1, &tbo_);

  tbo_ = std::exchange(o.tbo_, 0);
  mem::Move(mem::kTexture, &o, this);
  return *this;
}

void Texture::Bind(GLuint unit) const { glBindTextureUnit(unit, tbo_); }

void Texture::SetStorage2D(GLsizei levels, GLenum internal_format,
                           GLsizei width, GLsizei height) const {
  glTextureStorage2D(tbo_, levels, internal_format, width, height);
  auto bytes = mem::CalcTexBytes2D(internal_format, levels, width * height);
  mem::Add(mem::kTexture, this, bytes);
}

void Texture::SetStorage2D(GLsizei levels, GLenum internal_format,
                           glm::ivec2 size) const {
  glTextureStorage2D(tbo_, levels, internal_format, size.x, size.y);
  auto bytes = mem::CalcTexBytes2D(internal_format, levels, size.x * size.y);
  mem::Add(mem::kTexture, this, bytes);
}

void Texture::SetStorage3D(GLsizei levels, GLenum internal_format,
                           GLsizei width, GLsizei height, GLsizei depth) const {
  glTextureStorage3D(tbo_, levels, internal_format, width, height, depth);
  auto bytes =
      mem::CalcTexBytes3D(internal_format, levels, width * height * depth);
  mem::Add(mem::kTexture, this, bytes);
}

void Texture::SetStorage3D(GLsizei levels, GLenum internal_format,
                           glm::ivec2 size, GLsizei depth) const {
  glTextureStorage3D(tbo_, levels, internal_format, size.x, size.y, depth);
  auto bytes =
      mem::CalcTexBytes3D(internal_format, levels, size.x * size.y * depth);
  mem::Add(mem::kTexture, this, bytes);
}

void Texture::SetFilter(GLenum min_filter, GLenum mag_filter) const {
  glTextureParameteri(tbo_, GL_TEXTURE_MIN_FILTER, min_filter);
  glTextureParameteri(tbo_, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void Texture::SetAnisotropy(GLfloat value) const {
  glTextureParameterf(tbo_, GL_TEXTURE_MAX_ANISOTROPY, value);
}

void Texture::SetLoadBias(GLfloat value) const {
  glTextureParameterf(tbo_, GL_TEXTURE_LOD_BIAS, value);
}

void Texture::SetWrap2D(GLenum wrap) const {
  glTextureParameteri(tbo_, GL_TEXTURE_WRAP_S, wrap);
  glTextureParameteri(tbo_, GL_TEXTURE_WRAP_T, wrap);
}

void Texture::SetWrap3D(GLenum wrap) const {
  glTextureParameteri(tbo_, GL_TEXTURE_WRAP_S, wrap);
  glTextureParameteri(tbo_, GL_TEXTURE_WRAP_T, wrap);
  glTextureParameteri(tbo_, GL_TEXTURE_WRAP_R, wrap);
}

void Texture::SetBorderColor(const glm::vec4 &color) const {
  glTextureParameterfv(tbo_, GL_TEXTURE_BORDER_COLOR, &color[0]);
}

void Texture::EnableCompareMode(GLenum compare_func) const {
  glTextureParameteri(tbo_, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  glTextureParameteri(tbo_, GL_TEXTURE_COMPARE_FUNC, compare_func);
}

void Texture::DisableCompareMode() const {
  glTextureParameteri(tbo_, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

void Texture::GenerateMipMap() const { glGenerateTextureMipmap(tbo_); }

void Texture::SubImage2D(GLsizei width, GLsizei height, GLenum format,
                         GLenum type, const void *pixels) const {
  glTextureSubImage2D(tbo_, 0, 0, 0, width, height, format, type, pixels);
}

void Texture::SubImage2D(GLint level, GLint xoffset, GLint yoffset,
                         GLsizei width, GLsizei height, GLenum format,
                         GLenum type, const void *pixels) const {
  glTextureSubImage2D(tbo_, level, xoffset, yoffset, width, height, format,
                      type, pixels);
}

void Texture::SubImage3D(GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, const void *pixels) const {
  glTextureSubImage3D(tbo_, 0, 0, 0, 0, width, height, depth, format, type,
                      pixels);
}

void Texture::SubImage3D(GLint level, GLint xoffset, GLint yoffset,
                         GLint zoffset, GLsizei width, GLsizei height,
                         GLsizei depth, GLenum format, GLenum type,
                         const void *pixels) const {
  glTextureSubImage3D(tbo_, level, xoffset, yoffset, zoffset, width, height,
                      depth, format, type, pixels);
}

Texture2D::Texture2D() : Texture(GL_TEXTURE_2D) {}

Texture3D::Texture3D() : Texture(GL_TEXTURE_3D) {}

TextureCubeMap::TextureCubeMap() : Texture(GL_TEXTURE_CUBE_MAP) {}

Texture2DArray::Texture2DArray() : Texture(GL_TEXTURE_2D_ARRAY) {}

TextureCubeMapArray::TextureCubeMapArray()
    : Texture(GL_TEXTURE_CUBE_MAP_ARRAY) {}

}  // namespace gl