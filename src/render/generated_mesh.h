#pragma once

// global
#include <array>
// local
#include "global.h"
#include "opengl/vertex_array.h"

struct VertexCount {
  constexpr VertexCount() {}
  constexpr VertexCount(GLuint vertex, GLuint indice)
      : vertex_count(vertex), indice_count(indice) {}
  constexpr VertexCount operator+(const VertexCount &rhs) const {
    return {vertex_count + rhs.vertex_count, indice_count + rhs.indice_count};
  }

  GLuint vertex_count{0};
  GLuint indice_count{0};
};

// calc vertex/indice count
inline constexpr VertexCount CalcDebugBox() { return {8, 24}; }

inline constexpr VertexCount CalcCube() { return {24, 36}; }

inline constexpr VertexCount CalcSphere(unsigned int slices) {
  unsigned int parallels = static_cast<unsigned int>(slices * 0.5f);
  GLuint vertex_count = (parallels + 1) * (slices + 1);
  GLuint indice_count = parallels * slices * 6;
  return {vertex_count, indice_count};
}

inline constexpr VertexCount CalcCone(unsigned int slices,
                                      unsigned int stacks) {
  GLuint vertex_count = ((stacks + 1) * (slices + 1)) + (slices + 1) + 1;
  GLuint indice_count = (stacks * slices * 6) + (slices * 3);
  return {vertex_count, indice_count};
}

inline constexpr VertexCount CalcCylinder(unsigned int slices) {
  GLuint vertex_count = ((slices + 1) * 4) + 2;
  GLuint indice_count = slices * 12;
  return {vertex_count, indice_count};
}

inline constexpr VertexCount CalcPlane(unsigned int slices,
                                       unsigned int stacks) {
  GLuint vertex_count = (stacks + 1) * (slices + 1);
  GLuint indice_count = stacks * slices * 6;
  return {vertex_count, indice_count};
}

// calc buffer size
inline constexpr VertexCount CalcInternal() {
  VertexCount cube = CalcCube();
  VertexCount sphere = CalcSphere(global::kSlices);
  VertexCount box = CalcDebugBox();
  return {cube + sphere + box};
}

inline constexpr VertexCount CalcStatic() {
  VertexCount cube = CalcCube();
  VertexCount sphere = CalcSphere(global::kSlices);
  VertexCount cone = CalcCone(global::kSlices, global::kStacks);
  VertexCount cylinder = CalcCylinder(global::kSlices);
  VertexCount plane =
      CalcPlane(global::kPlaneSlicesStacks, global::kPlaneSlicesStacks);
  return {cube + sphere + cone + cylinder + plane};
}

inline constexpr VertexCount kInternalSize = CalcInternal();
inline constexpr VertexCount kStaticSize = CalcStatic();

struct GenMeshInternal {
  enum Enum : GLuint { kSkybox, kPointLight, kDebugBox, kTotal };
};

struct GenMeshStatic {
  enum Enum : GLuint { kCube, kSphere, kCone, kCylinder, kPlane, kTotal };
};

class GeneratedMesh {
 public:
  GeneratedMesh();

 public:
  gl::VertexBuffers mesh_internal_;
  gl::VertexBuffers mesh_static_;
  gl::VertexArray pos_;
  gl::VertexArray pos_norm_tex_;

  void DrawInternalMesh(GenMeshInternal::Enum type, GLenum mode) const;
  void DrawInternalMeshInstanced(GenMeshInternal::Enum type, GLenum mode,
                                 GLuint count) const;
  void DrawStaticMesh(GenMeshStatic::Enum type, GLenum mode) const;

  const gl::VertexAddr &GetVertexAddr(GenMeshInternal::Enum type) const;
  const gl::VertexAddr &GetVertexAddr(GenMeshStatic::Enum type) const;

 private:
  std::array<gl::VertexAddr, GenMeshInternal::kTotal> addr_internal_;
  std::array<gl::VertexAddr, GenMeshStatic::kTotal> addr_static_;

  void GenDebugBox(float radius);
  void GenSkybox(float radius);
  void GenPointLight(float radius, unsigned int slices);
  void GenCube(float radius, float tex_coords_scale);
  void GenSphere(float radius, unsigned int slices);
  void GenCone(float height, float radius, unsigned int slices,
               unsigned int stacks);
  void GenCylinder(float height, float radius, unsigned int slices);
  void GenPlane(float width, float height, unsigned int slices,
                unsigned int stacks);

  struct VertexDataInternal {
    VertexDataInternal(VertexCount count) {
      positions.reserve(count.vertex_count);
      indices.reserve(count.indice_count);
    }
    MiVector<glm::vec3> positions;
    MiVector<unsigned int> indices;
  };

  struct VertexDataStatic : public VertexDataInternal {
    VertexDataStatic(VertexCount count) : VertexDataInternal(count) {
      normals.reserve(count.vertex_count);
      tangents.reserve(count.vertex_count);
      tex_coords.reserve(count.vertex_count);
    }
    MiVector<glm::vec3> normals;
    MiVector<glm::vec3> tangents;
    MiVector<glm::vec2> tex_coords;
  };

  void CalcTangents(VertexDataStatic &data);
  void UploadMeshInternal(VertexDataInternal &data, GenMeshInternal::Enum type);
  void UploadMeshStatic(VertexDataStatic &data, GenMeshStatic::Enum type);
};