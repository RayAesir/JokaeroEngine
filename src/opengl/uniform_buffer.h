#pragma once

// deps
#include <glad/glad.h>
// local
#include "mi_types.h"

namespace gl {

// single buffer, GL_MAX_UNIFORM_BLOCK_SIZE (~16-64KB)
class UniformBuffer {
 public:
  UniformBuffer() noexcept;
  ~UniformBuffer();
  // OpenGL resources (move-only)
  UniformBuffer(const UniformBuffer &) = delete;
  UniformBuffer &operator=(const UniformBuffer &) = delete;
  UniformBuffer(UniformBuffer &&o) noexcept;
  UniformBuffer &operator=(UniformBuffer &&) noexcept;

 public:
  template <typename T>
  T *MapBuffer(GLuint binding) {
    GLsizeiptr size = sizeof(T);
    void *ptr = MapBufferImpl(size, binding);
    return static_cast<T *>(ptr);
  }
  void Upload() const;
  void SubData(GLuint binding, GLsizeiptr size, void *data) const;

 private:
  GLuint ubo_{0};
  MiVector<GLintptr> offsets_;
  MiVector<unsigned char> raw_memory_;
  void *MapBufferImpl(GLsizeiptr size, GLuint binding) noexcept;
};

}  // namespace gl