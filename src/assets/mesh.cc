#include "mesh.h"

// deps
#include <assimp/scene.h>
#include <spdlog/spdlog.h>
// local
#include "assets/animation.h"
#include "assets/assimp_blender.h"
#include "assets/model_manager.h"
#include "files.h"
#include "math/assimp_to_glm.h"
#include "utils/string_parsing.h"

MeshInfo::MeshInfo(const aiMesh *mesh) {
  vertex_count = mesh->mNumVertices;
  unsigned int indices_per_face = mesh->mFaces[0].mNumIndices;
  if (indices_per_face > 3) {
    indices_per_face = 3;
    spdlog::warn("{}: Mesh is not triangulated '{}'", __FUNCTION__,
                 mesh->mName.C_Str());
  }
  indice_count = mesh->mNumFaces * indices_per_face;

  buffer_type = 0;
  if ((vertex_count > 0) && (indice_count > 0)) {
    buffer_type |= MeshInfo::kPosition;
  }
  if (mesh->HasNormals()) {
    buffer_type |= MeshInfo::kNormals;
  }
  if (mesh->HasTangentsAndBitangents()) {
    buffer_type |= MeshInfo::kTangentsBitangents;
  }
  if (mesh->HasTextureCoords(0)) {
    buffer_type |= MeshInfo::kTexCoords;
  }
  if (mesh->HasBones()) {
    buffer_type |= MeshInfo::kBones;
  }
}

Mesh::Mesh(aiMesh *mesh, aiMaterial *material, const MeshInfo &load_info,
           ModelManager &models) {
  ProcessMesh(mesh, material, load_info);
  UploadStatic(mesh, models.mesh_static_);
  ProcessTextures(material, models.textures_);
}

Mesh::Mesh(aiMesh *mesh, aiMaterial *material, const MeshInfo &load_info,
           Skeleton &skeleton, ModelManager &models) {
  ProcessMesh(mesh, material, load_info);
  UploadSkinned(mesh, skeleton, models.mesh_skinned_);
  ProcessTextures(material, models.textures_);
}

void Mesh::ProcessMesh(aiMesh *mesh, aiMaterial *material,
                       const MeshInfo &load_info) {
  name_ =
      fmt::format("{}: {}", mesh->mName.C_Str(), material->GetName().C_Str());

  addr_.vertex_count = load_info.vertex_count;
  addr_.indice_count = load_info.indice_count;

  if (load_info.buffer_type == BufferType::kSkinned) {
    type_ = MeshType::kSkinnedOpaque;
  } else {
    type_ = MeshType::kStaticOpaque;
  }

  bb_ =
      AABB(assglm::GetVec(mesh->mAABB.mMin), assglm::GetVec(mesh->mAABB.mMax));
}

void Mesh::ExtractIndices(aiMesh *mesh, MiVector<unsigned int> &indices) {
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    aiFace &face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }
}

void Mesh::UploadStatic(aiMesh *mesh, gl::VertexBuffers &buffers) noexcept {
  GLuint vertex_count = addr_.vertex_count;
  GLuint indice_count = addr_.indice_count;

  // mTextureCoords as vec3, sadly
  MiVector<float> tex_coords;
  tex_coords.reserve(vertex_count * 2);
  for (GLuint i = 0; i < vertex_count; ++i) {
    tex_coords.push_back(mesh->mTextureCoords[0][i].x);
    tex_coords.push_back(mesh->mTextureCoords[0][i].y);
  }

  MiVector<void *> vertices = {
      static_cast<void *>(mesh->mVertices),
      static_cast<void *>(mesh->mNormals),
      static_cast<void *>(mesh->mTangents),
      static_cast<void *>(mesh->mBitangents),
      static_cast<void *>(tex_coords.data()),
  };

  MiVector<unsigned int> indices;
  indices.reserve(indice_count);
  ExtractIndices(mesh, indices);

  addr_ = buffers.UploadVerticesIndicesMt(vertices, vertex_count,
                                          static_cast<void *>(indices.data()),
                                          indice_count);
}

// quite fast, under 1ms avg
void Mesh::ExtractBoneWeight(aiMesh *mesh, Skeleton &skeleton,
                             MiVector<glm::ivec4> &bones,
                             MiVector<glm::vec4> &weights) {
  auto &bone_map = skeleton.bone_map;
  auto &bone_to_local = skeleton.bone_to_local;
  auto &bone_box = skeleton.bone_box;

  auto &bones_per_mesh = skeleton.bones_per_model.emplace_back();
  bones_per_mesh.reserve(mesh->mNumBones);

  for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
    int bone_id = 0;
    aiBone *bone = mesh->mBones[b];
    std::string bone_name{bone->mName.C_Str()};

    if (bone_map.contains(bone_name)) {
      bone_id = bone_map.at(bone_name);
    } else {
      bone_id = static_cast<int>(bone_map.size());
      bone_map.try_emplace(bone_name, bone_id);
      bone_to_local.emplace_back(assglm::GetMatrix(bone->mOffsetMatrix));
    }

    aiVertexWeight *vertex_weights = bone->mWeights;
    unsigned int num_weights = bone->mNumWeights;

    // Assimp/fbx doesn't have precomputed volumes around bones
    AABB bb;
    for (unsigned int w = 0; w < num_weights; ++w) {
      unsigned int vertex_id = vertex_weights[w].mVertexId;
      float weight = vertex_weights[w].mWeight;

      // build box around bone vertices
      bb.ExpandToInclude(assglm::GetVec(mesh->mVertices[vertex_id]));

      // check a free slot
      for (int i = 0; i < global::kMaxBonesPerVertex; ++i) {
        if (weights[vertex_id][i] == 0.0f) {
          bones[vertex_id][i] = bone_id;
          weights[vertex_id][i] = weight;
          break;
        }
      }
    }

    // one bone can overlap two different meshes
    auto it = bone_box.find(bone_id);
    if (it != bone_box.end()) {
      it->second.ExpandToInclude(bb);
    } else {
      bone_box.try_emplace(bone_id, bb);
    }

    // record list of bone per mesh
    bones_per_mesh.push_back(bone_id);
  }
}

void Mesh::UploadSkinned(aiMesh *mesh, Skeleton &skeleton,
                         gl::VertexBuffers &buffers) noexcept {
  GLuint vertex_count = addr_.vertex_count;
  GLuint indice_count = addr_.indice_count;

  // mTextureCoords as vec3, sadly
  MiVector<float> tex_coords;
  tex_coords.reserve(vertex_count * 2);
  for (GLuint i = 0; i < vertex_count; ++i) {
    tex_coords.push_back(mesh->mTextureCoords[0][i].x);
    tex_coords.push_back(mesh->mTextureCoords[0][i].y);
  }

  MiVector<glm::ivec4> bones(vertex_count, glm::ivec4(0));
  MiVector<glm::vec4> weights(vertex_count, glm::vec4(0.0f));
  ExtractBoneWeight(mesh, skeleton, bones, weights);

  MiVector<void *> vertices = {
      static_cast<void *>(mesh->mVertices),
      static_cast<void *>(mesh->mNormals),
      static_cast<void *>(mesh->mTangents),
      static_cast<void *>(mesh->mBitangents),
      static_cast<void *>(tex_coords.data()),
      static_cast<void *>(bones.data()),
      static_cast<void *>(weights.data()),
  };

  MiVector<unsigned int> indices;
  indices.reserve(indice_count);
  ExtractIndices(mesh, indices);

  addr_ = buffers.UploadVerticesIndicesMt(vertices, vertex_count,
                                          static_cast<void *>(indices.data()),
                                          indice_count);
}

// check the material name to recognize a base type
// e.g. Material.AlphaBlend (Blender dot notation)
// or alphaclip_material_name
void Mesh::CheckTransparency(const char *mat_name) {
  std::string mat_str{mat_name};
  parse::ToLower(mat_str);

  // by default MeshType is Opaque [0, 1]
  if (mat_str.find("alphaclip") != std::string::npos) {
    type_ = static_cast<MeshType::Enum>(type_ + MeshType::kStaticAlphaClip);
    return;
  }
  if (mat_str.find("alphablend") != std::string::npos) {
    type_ = static_cast<MeshType::Enum>(type_ + MeshType::kStaticAlphaBlend);
    return;
  }
}

void Mesh::ProcessTextures(aiMaterial *material, TextureManager &textures) {
  // data comes from Blender's BSDF node
  // FBX doesn't support PBR well, but Blender exports all we need anyway
  // source: export_fbx_bin.py

  // update type (opaque -> alpha_clip/alpha_blend)
  CheckTransparency(material->GetName().C_Str());

  aiColor3D color_diffuse;
  float metallic_factor;
  float roughness_factor;
  aiColor3D color_emission;
  // Blender exports emission strength but Assimp doesn't load, bug?
  constexpr float emission_factor = 1.0f;
  float transparency_factor;
  material->Get(AI_MATKEY_COLOR_DIFFUSE, color_diffuse);
  material->Get(AI_MATKEY_REFLECTIVITY, metallic_factor);
  material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor);
  material->Get(AI_MATKEY_COLOR_EMISSIVE, color_emission);
  //   material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emission_factor);
  material->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency_factor);

  material_.diffuse_color_alpha_threshold = glm::vec4(
      color_diffuse.r, color_diffuse.g, color_diffuse.b, transparency_factor);

  material_.roughness = roughness_factor;
  material_.metallic = metallic_factor;

  material_.emission_color_emission_intensity = glm::vec4(
      color_emission.r, color_emission.g, color_emission.b, emission_factor);

  std::array<aiString, TextureType::kTotal> paths;
  material->Get(AI_MATKEY_FBX_BLENDER_DIFFUSE_TEXTURE,
                paths[TextureType::kDiffuse]);
  material->Get(AI_MATKEY_FBX_BLENDER_METALLIC_TEXTURE,
                paths[TextureType::kMetallic]);
  material->Get(AI_MATKEY_FBX_BLENDER_ROUGHNESS_TEXTURE,
                paths[TextureType::kRoughness]);
  material->Get(AI_MATKEY_FBX_BLENDER_EMISSIVE_TEXTURE,
                paths[TextureType::kEmissive]);
  material->Get(AI_MATKEY_FBX_BLENDER_EMISSIVE_FACTOR_TEXTURE,
                paths[TextureType::kEmissiveFactor]);
  material->Get(AI_MATKEY_FBX_BLENDER_NORMALS_TEXTURE,
                paths[TextureType::kNormals]);

  for (GLuint i = 0; i < TextureType::kTotal; ++i) {
    if (paths[i].length == 0) continue;

    const std::string path = fmt::format("resources/{}", paths[i].C_Str());
    textures_[i] =
        textures.CreateTextureMt(path, static_cast<TextureType::Enum>(i));
    material_.handlers[i] = textures_[i]->GetHandler();
    material_.tex_flags |= (1U << i);
  }

  // no diffuse texture - no alpha channel
  // can be AlphaBlend only
  if (paths[TextureType::kDiffuse].length == 0) {
    if (type_ == MeshType::kStaticAlphaClip) {
      type_ = MeshType::kStaticAlphaBlend;
    }
    if (type_ == MeshType::kSkinnedAlphaClip) {
      type_ = MeshType::kSkinnedAlphaBlend;
    }
  }
}

void Mesh::PrintProps(aiMaterial *material) const {
  for (unsigned int i = 0; i < material->mNumProperties; ++i) {
    aiMaterialProperty *prop = material->mProperties[i];
    std::string key;
    std::string info{prop->mKey.C_Str()};
    switch (prop->mType) {
      case aiPTI_Float:
        key = "float";
        break;
      case aiPTI_Double:
        key = "double";
        break;
      case aiPTI_String:
        key = "string";
        break;
      case aiPTI_Integer:
        key = "int";
        break;
      case aiPTI_Buffer:
        key = "binary buffer";
        break;
      default:
        key = "unknown";
        break;
    }
    spdlog::info("Material Props: {}: {} key = {}", material->GetName().C_Str(),
                 key, info);
  }
}

const std::string &Mesh::GetName() const { return name_; }

MeshType::Enum Mesh::GetType() const { return type_; }

const gl::VertexAddr &Mesh::GetVertexAddr() const { return addr_; }

const Textures &Mesh::GetTextures() const { return textures_; }

const AABB &Mesh::GetBB() const { return bb_; }

void Mesh::UpdateMaterialTextureHandlers() {
  for (int i = 0; i < TextureType::kTotal; ++i) {
    if (textures_[i] == nullptr) continue;
    material_.handlers[i] = textures_[i]->GetHandler();
  }
}
