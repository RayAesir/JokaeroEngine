#pragma once

// local
#include "assets/animation.h"
#include "assets/mesh.h"
#include "id_generator.h"
// fwd
class ModelManager;
class TextureManager;
struct aiScene;

class Model {
 public:
  Model(const std::string &name, const aiScene *scene, ModelManager &models);

 public:
  MiVector<Mesh> meshes_;

  id::Model GetId() const;
  const std::string &GetName() const;
  const AABB &GetBB() const;

  GLuint GetInstanceCount() const;
  GLuint GetFirstBoxBufferIndex() const;

  bool HasAnimations() const;
  Animation *GetAnimationByIndex(size_t index);
  const MiVector<Animation> &GetAnimations() const;

  const Skeleton &GetSkeleton() const;
  GLuint GetNumOfBones() const;

 private:
  id::Model id_;
  std::string name_;
  // bounding box around model (to place in the Scene)
  AABB bb_;

  MiVector<Animation> animations_;
  Skeleton skeleton_;

  void LoadStaticMeshes(const aiScene *scene, ModelManager &models);

  void LoadSkinnedMeshes(Skeleton &skeleton, const aiScene *scene,
                         ModelManager &models);
  void LoadAnimations(Skeleton &skeleton, const aiScene *scene);
};
