#pragma once

// deps
#include <glad/glad.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace gl {

class Texture {
 public:
  Texture() = default;
  Texture(GLenum target);
  ~Texture();
  // OpenGL resources (move-only)
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&o) noexcept;
  Texture &operator=(Texture &&o) noexcept;

 public:
  operator GLuint() const { return tbo_; }
  void Bind(GLuint unit) const;
  void SetStorage2D(GLsizei levels, GLenum internal_format, GLsizei width,
                    GLsizei height) const;
  void SetStorage2D(GLsizei levels, GLenum internal_format,
                    glm::ivec2 size) const;
  void SetStorage3D(GLsizei levels, GLenum internal_format, GLsizei width,
                    GLsizei height, GLsizei depth) const;
  void SetStorage3D(GLsizei levels, GLenum internal_format, glm::ivec2 size,
                    GLsizei depth) const;
  void SetFilter(GLenum min_filter = GL_NEAREST_MIPMAP_LINEAR,
                 GLenum mag_filter = GL_LINEAR) const;
  // float, OpenGL 4.6, (GL_LINEAR_MIPMAP_LINEAR)
  void SetAnisotropy(GLfloat value) const;
  // positive - blur, negative - sharp
  void SetLoadBias(GLfloat value) const;
  void SetWrap2D(GLenum wrap = GL_REPEAT) const;
  void SetWrap3D(GLenum wrap = GL_REPEAT) const;
  void SetBorderColor(const glm::vec4 &color) const;
  // internal format should be GL_DEPTH_COMPONENT
  void EnableCompareMode(GLenum compare_func = GL_LEQUAL) const;
  void DisableCompareMode() const;
  void GenerateMipMap() const;
  void SubImage2D(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const void *pixels) const;
  void SubImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                  GLsizei height, GLenum format, GLenum type,
                  const void *pixels) const;
  void SubImage3D(GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                  GLenum type, const void *pixels) const;
  void SubImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                  GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                  GLenum type, const void *pixels) const;

 protected:
  GLuint tbo_{0};
};

// most common presets to improve readability
class Texture2D : public Texture {
 public:
  // hint: SetStorage2D() + SetWrap2D()
  Texture2D();
};

class Texture3D : public Texture {
 public:
  // hint: SetStorage3D() + SetWrap3D()
  Texture3D();
};

class TextureCubeMap : public Texture {
 public:
  // hint: SetStorage2D() + SetWrap3D()
  TextureCubeMap();
};

class Texture2DArray : public Texture {
 public:
  // hint: SetStorage3D() + SetWrap2D()
  Texture2DArray();
};

class TextureCubeMapArray : public Texture {
 public:
  // hint: SetStorage3D() + SetWrap3D()
  // cubemaps array use layer-faces, 6 * depth
  TextureCubeMapArray();
};

}  // namespace gl