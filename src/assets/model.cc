#include "model.h"

// deps
#include <assimp/scene.h>
#include <spdlog/spdlog.h>
// local
#include "assets/model_manager.h"

bool WrongBuffer(GLuint check_type, GLuint buffer_type, aiMesh *mesh) {
  if (check_type == buffer_type) return false;
  spdlog::error("{}: '{}'", __FUNCTION__, mesh->mName.C_Str());
  return true;
}

void Model::LoadStaticMeshes(const aiScene *scene, ModelManager &models) {
  meshes_.reserve(scene->mNumMeshes);
  auto &buffer = models.mesh_static_;

  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[i];
    MeshInfo info{mesh};
    if (WrongBuffer(BufferType::kStatic, info.buffer_type, mesh)) continue;
    if (buffer.NotEnoughSpaceMt(info.vertex_count, info.indice_count)) continue;

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    meshes_.emplace_back(mesh, material, info, models);
  }
}

Skeleton PrepareSkeleton(const aiScene *scene) {
  unsigned int total_bones = 0;
  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[i];
    total_bones += mesh->mNumBones;
  }
  return Skeleton{total_bones};
}

void Model::LoadSkinnedMeshes(Skeleton &skeleton, const aiScene *scene,
                              ModelManager &models) {
  // the scene contains all data, the nodes is just to keep stuff organized
  meshes_.reserve(scene->mNumMeshes);
  auto &buffer = models.mesh_skinned_;

  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[i];
    MeshInfo info{mesh};
    if (WrongBuffer(BufferType::kSkinned, info.buffer_type, mesh)) continue;
    if (buffer.NotEnoughSpaceMt(info.vertex_count, info.indice_count)) continue;

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    meshes_.emplace_back(mesh, material, info, skeleton, models);
  }
}

void Model::LoadAnimations(Skeleton &skeleton, const aiScene *scene) {
  aiNode *root_node = scene->mRootNode;
  animations_.reserve(scene->mNumAnimations);
  for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
    aiAnimation *animation = scene->mAnimations[i];
    animations_.emplace_back(animation, root_node, skeleton);
  }
}

Model::Model(const std::string &name, const aiScene *scene,
             ModelManager &models)
    : id_(id::GenId(id::kModel)), name_(name) {
  if (scene->HasAnimations()) {
    Skeleton skeleton = PrepareSkeleton(scene);
    LoadSkinnedMeshes(skeleton, scene, models);
    // load animations when Skeleton processed
    LoadAnimations(skeleton, scene);
    skeleton_ = std::move(skeleton);
  } else {
    LoadStaticMeshes(scene, models);
  }

  if (meshes_.size() == 0) return;

  // calculate model's AABB
  bb_ = meshes_[0].GetBB();
  for (size_t box = 1; box < meshes_.size(); ++box) {
    bb_.ExpandToInclude(meshes_[box].GetBB());
  }
}

id::Model Model::GetId() const { return id_; }

const std::string &Model::GetName() const { return name_; };

const AABB &Model::GetBB() const { return bb_; }

GLuint Model::GetInstanceCount() const {
  return static_cast<GLuint>(meshes_.size());
}

GLuint Model::GetFirstBoxBufferIndex() const { return meshes_[0].box_index_; }

bool Model::HasAnimations() const {
  return (animations_.size()) ? true : false;
}

Animation *Model::GetAnimationByIndex(size_t index) {
  return (index < animations_.size()) ? &animations_[index] : nullptr;
}

const MiVector<Animation> &Model::GetAnimations() const { return animations_; }

const Skeleton &Model::GetSkeleton() const { return skeleton_; }

GLuint Model::GetNumOfBones() const {
  return static_cast<GLuint>(skeleton_.bone_map.size());
}