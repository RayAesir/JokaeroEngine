#pragma once

// global
#include <memory>
// local
#include "assets/texture.h"
#include "global.h"
#include "math/collision_types.h"
#include "opengl/vertex_buffers.h"
// fwd
class ModelManager;
class TextureManager;
struct Skeleton;
struct aiMesh;
struct aiMaterial;

namespace gpu {

// PBR
struct Material {
  // loaded textures from meshes
  GLuint64 handlers[TextureType::kTotal];
  // flags to check textures
  GLuint tex_flags{0};
  GLfloat roughness;
  GLfloat metallic;
  GLfloat empty;
  //
  glm::vec4 diffuse_color_alpha_threshold;
  glm::vec4 emission_color_emission_intensity;
};

}  // namespace gpu

struct MeshInfo {
  MeshInfo(const aiMesh *mesh);
  enum Enum : GLuint {
    kPosition = (1 << 0),            //
    kNormals = (1 << 1),             //
    kTangentsBitangents = (1 << 2),  //
    kTexCoords = (1 << 3),           //
    kBones = (1 << 4)                //
  };
  GLuint vertex_count;
  GLuint indice_count;
  GLuint buffer_type;
};

struct BufferType {
  enum Enum : GLuint {
    kStatic = MeshInfo::kPosition |            //
              MeshInfo::kNormals |             //
              MeshInfo::kTangentsBitangents |  //
              MeshInfo::kTexCoords,            //
    kSkinned = kStatic | MeshInfo::kBones
  };
};

using Textures = std::array<std::shared_ptr<SmartTexture>, TextureType::kTotal>;

class Mesh {
 public:
  Mesh(aiMesh *mesh, aiMaterial *material, const MeshInfo &load_info,
       ModelManager &models);
  Mesh(aiMesh *mesh, aiMaterial *material, const MeshInfo &load_info,
       Skeleton &skeleton, ModelManager &models);

 public:
  gpu::Material material_;
  // set by Manager (batch upload to buffer)
  GLuint material_buffer_index_{0};
  GLuint packed_cmd_buffer_index_{0};
  GLuint box_index_{0};

  const std::string &GetName() const;
  MeshType::Enum GetType() const;

  const gl::VertexAddr &GetVertexAddr() const;
  const Textures &GetTextures() const;
  const AABB &GetBB() const;

  void UpdateMaterialTextureHandlers();

 private:
  std::string name_;
  MeshType::Enum type_;
  gl::VertexAddr addr_;
  Textures textures_{};
  AABB bb_;

  void ProcessMesh(aiMesh *mesh, aiMaterial *material,
                   const MeshInfo &load_info);
  void ExtractIndices(aiMesh *mesh, MiVector<unsigned int> &indices);
  void UploadStatic(aiMesh *mesh, gl::VertexBuffers &buffers) noexcept;
  void ExtractBoneWeight(aiMesh *mesh, Skeleton &skeleton,
                         MiVector<glm::ivec4> &bones,
                         MiVector<glm::vec4> &weights);
  void UploadSkinned(aiMesh *mesh, Skeleton &skeleton,
                     gl::VertexBuffers &buffers) noexcept;
  void CheckTransparency(const char *mat_name);
  void ProcessTextures(aiMaterial *material, TextureManager &textures);
  // debug
  void PrintProps(aiMaterial *material) const;
};