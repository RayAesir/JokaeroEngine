#pragma once

// deps
#include <glad/glad.h>

#include <glm/mat4x4.hpp>
// global
#include <mutex>
#include <string>
// local
#include "mi_types.h"
#include "opengl/fence_sync.h"

namespace gl {

// OpenGL supports at minimum 16 vertex attributes
// vertex shader: layout (location = VertexAttribute) in;
struct VertexLocation {
  enum Enum : GLuint {
    kPosition,
    kNormal,
    kTangent,
    kBitangent,
    kTexCoords,
    kVertexColor,
    kBones,
    kWeights,
    kInstanceAttr,
    kInstanceMatrix,
  };
};

struct VertexAttribute {
  const char *name;
  GLuint index;
  GLuint num_of_comp;
  GLint type;
  GLuint stride;
  GLuint divisor;
};

using Attributes = MiVector<const VertexAttribute *>;

void CheckAttributes(Attributes &attrs);

struct VertexAddr {
  GLuint vertex_count;
  GLuint vertex_offset;
  GLuint indice_count;
  // offset in units, for Indirect CMD
  GLuint indice_offset;
  // offset in bytes, GLuint64 as pointer, for glDraw
  GLvoid *first_index;

  // bind VAO one time, draw multiple
  void Draw(GLenum mode) const;
  void DrawInstanced(GLenum mode, GLuint count) const;
  void DrawInstancedBaseInstance(GLenum mode, GLuint count, GLuint base) const;
};

class VertexBuffers {
 public:
  VertexBuffers(const Attributes &attrs);
  ~VertexBuffers();
  // OpenGL resources, Mutex (move-only, not thread-safe)
  VertexBuffers(const VertexBuffers &) = delete;
  VertexBuffers &operator=(VertexBuffers &) = delete;
  VertexBuffers(VertexBuffers &&o) noexcept;
  VertexBuffers &operator=(VertexBuffers &&o) noexcept;

 public:
  void SetStorage(GLuint vertex_count, GLuint indice_count);

  const Attributes &GetAttrs() const;
  const MiVector<GLuint> &GetBuffers() const;
  GLuint GetEbo() const;

  bool NotEnoughSpaceMt(GLuint vertex_count, GLuint indice_count) const;
  VertexAddr UploadVerticesIndicesMt(const MiVector<void *> &vertices,
                                     GLuint vertex_count, void *indices,
                                     GLuint indice_count);

 private:
  Attributes attrs_;
  MiVector<GLuint> buffers_;
  MiVector<GLsizei> strides_;
  GLuint ebo_{0};

  GLuint vertex_current_count_{0};
  GLuint indice_current_count_{0};
  GLuint vertex_total_count_{0};
  GLuint indice_total_count_{0};

  mutable std::mutex mutex_;
  gl::Sync sync_;
};

}  // namespace gl

// declarative style to reduce boilerplate and typing errors
inline const gl::VertexAttribute kPositionAttr{
    .name = "Position",                      //
    .index = gl::VertexLocation::kPosition,  //
    .num_of_comp = glm::vec3::length(),      //
    .type = GL_FLOAT,                        //
    .stride = sizeof(glm::vec3),             //
    .divisor = 0,                            //
};

inline const gl::VertexAttribute kNormalAttr{
    .name = "Normal",                      //
    .index = gl::VertexLocation::kNormal,  //
    .num_of_comp = glm::vec3::length(),    //
    .type = GL_FLOAT,                      //
    .stride = sizeof(glm::vec3),           //
    .divisor = 0,                          //
};

inline const gl::VertexAttribute kTangentAttr{
    .name = "Tangent",                      //
    .index = gl::VertexLocation::kTangent,  //
    .num_of_comp = glm::vec3::length(),     //
    .type = GL_FLOAT,                       //
    .stride = sizeof(glm::vec3),            //
    .divisor = 0,                           //
};

inline const gl::VertexAttribute kBitangentAttr{
    .name = "Bitangent",                      //
    .index = gl::VertexLocation::kBitangent,  //
    .num_of_comp = glm::vec3::length(),       //
    .type = GL_FLOAT,                         //
    .stride = sizeof(glm::vec3),              //
    .divisor = 0,                             //
};

inline const gl::VertexAttribute kTexCoordsAttr{
    .name = "TexCoords",                      //
    .index = gl::VertexLocation::kTexCoords,  //
    .num_of_comp = glm::vec2::length(),       //
    .type = GL_FLOAT,                         //
    .stride = sizeof(glm::vec2),              //
    .divisor = 0,                             //
};

inline const gl::VertexAttribute kVertexColorAttr{
    .name = "VertexColor",                      //
    .index = gl::VertexLocation::kVertexColor,  //
    .num_of_comp = glm::vec3::length(),         //
    .type = GL_FLOAT,                           //
    .stride = sizeof(glm::vec3),                //
    .divisor = 0,                               //
};

inline const gl::VertexAttribute kBonesAttr{
    .name = "Bones",                      //
    .index = gl::VertexLocation::kBones,  //
    .num_of_comp = glm::ivec4::length(),  //
    .type = GL_INT,                       //
    .stride = sizeof(glm::ivec4),         //
    .divisor = 0,                         //
};

inline const gl::VertexAttribute kWeightsAttr{
    .name = "Weights",                      //
    .index = gl::VertexLocation::kWeights,  //
    .num_of_comp = glm::vec4::length(),     //
    .type = GL_FLOAT,                       //
    .stride = sizeof(glm::vec4),            //
    .divisor = 0,                           //
};

inline const gl::VertexAttribute kInstanceAttr{
    .name = "InstanceAttr",                      //
    .index = gl::VertexLocation::kInstanceAttr,  //
    .num_of_comp = glm::uvec4::length(),         //
    .type = GL_UNSIGNED_INT,                     //
    .stride = sizeof(glm::uvec4),                //
    .divisor = 1,                                //
};

inline const gl::VertexAttribute kInstanceMatrix{
    .name = "InstanceMatrix",                      //
    .index = gl::VertexLocation::kInstanceMatrix,  //
    .num_of_comp = glm::mat4::length(),            //
    .type = GL_FLOAT,                              //
    .stride = sizeof(glm::mat4),                   //
    .divisor = 1,                                  //
};
