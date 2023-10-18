#pragma once

// local
#include "opengl/vertex_buffers.h"

namespace gl {

// https://anteru.net/blog/2016/storing-vertex-data-to-interleave-or-not-to-interleave/
// non-interleaved vertex data
// the early-z and shadow passes are 20% slower with interleaved data (GTX 1080)
// meshes loading by Assimp is faster
class VertexArray {
 public:
  VertexArray();
  ~VertexArray();
  // OpenGL resources (move-only)
  VertexArray(const VertexArray &) = delete;
  VertexArray &operator=(const VertexArray &) = delete;
  VertexArray(VertexArray &&o) noexcept;
  VertexArray &operator=(VertexArray &&o) noexcept;

 public:
  operator GLuint() const { return vao_; }

  void SetAttributes(const Attributes &attrs);
  void SetVertexBuffers(const VertexBuffers &vertex_buffers);
  void SetBuffer(const VertexAttribute *attr, GLuint buffer);
  void AttachBuffers() const;
  void PrintVaoInfo() const;

  void Bind() const;
  const MiVector<GLuint> &GetBuffers() const;
  GLuint GetEbo() const;

 private:
  Attributes attrs_;
  MiVector<GLuint> buffers_;
  GLuint vao_;
  GLuint ebo_{0};

  void EnableVertexAttrib(const VertexAttribute *attr, GLuint binding);
  void EnableMatrixAttrib(const VertexAttribute *attr, GLuint binding);
  MiVector<GLsizei> GetStrides() const;
};

}  // namespace gl