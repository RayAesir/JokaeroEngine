#pragma once

// deps
#include <glm/gtc/quaternion.hpp>
// global
#include <span>
#include <string>
// local
#include "math/collision_types.h"
#include "mi_types.h"
// fwd
struct aiNode;
struct aiAnimation;
struct aiNodeAnim;

// created in two steps:
// 1. populated by processing Mesh (extract bones/boxes)
// 2. additional data (Nodes) append by Animation construction
struct Skeleton {
  Skeleton() = default;
  Skeleton(unsigned int total_bones) {
    bone_map.reserve(total_bones);
    bone_to_local.reserve(total_bones);
    bone_box.reserve(total_bones);
  }
  // to read animation's channels, check node hierarchy for bone
  MiUnMap<std::string, int> bone_map;
  // bone space to mesh space, already combined
  MiVector<glm::mat4> bone_to_local;
  // bone_id to AABB
  MiUnMap<int, AABB> bone_box;
  // bone indices per mesh
  MiVector<MiVector<int>> bones_per_model;
};

class Bone {
 public:
  Bone(int id, const aiNodeAnim* channel) noexcept;

 public:
  void Update(float animation_time);
  const glm::mat4& GetLocalTransform() const;
  int GetBoneID() const;

 private:
  int id_;
  MiVector<glm::vec3> positions;
  MiVector<glm::quat> rotations;
  MiVector<glm::vec3> scales;
  MiVector<float> positions_time;
  MiVector<float> rotations_time;
  MiVector<float> scales_time;
  unsigned int num_positions;
  unsigned int num_rotations;
  unsigned int num_scalings;
  glm::mat4 local_transform_;

  const int GetPositionIndex(float animation_time);
  const int GetRotationIndex(float animation_time);
  const int GetScaleIndex(float animation_time);
  glm::vec3 InterpolatePosition(float animation_time);
  glm::quat InterpolateRotation(float animation_time);
  glm::vec3 InterpolateScaling(float animation_time);
  float GetScaleFactor(float last_timestamp, float next_timestamp,
                       float animation_time);
};

// flatten data, from list to vector
struct NodeData {
  // -1 if node
  int bone_id{-1};
  // indentation if bone
  glm::mat4 transformation{1.0f};
  // indices inside vector
  MiVector<int> children;
};

// if import Blender .fbx via Assimp
// RootNode -> Armature -> other childrens
class Animation {
 public:
  Animation(aiAnimation* animation, aiNode* root_node,
            Skeleton& skeleton) noexcept;

 public:
  const std::string& GetName() const;

  float GetTicksPerSecond() const;
  float GetDuration() const;

  const MiVector<Bone>& GetBones() const;
  GLuint GetNumOfBones() const;
  gpu::AABB GetCurrentBox(GLuint mesh, float time) const;

  void PlayAnimation(const Skeleton& skeleton, float time,
                     std::span<glm::mat4> bone_mat) noexcept;

 private:
  std::string name_;

  int ticks_per_second_;
  float duration_;

  MiVector<NodeData> nodes_data_;
  MiVector<Bone> bones_;
  // per mesh, keyframe boxes, [mesh][tick]
  MiVector<MiVector<gpu::AABB>> boxes_;

  void ReadBonesData(const aiAnimation* animation, Skeleton& skeleton) noexcept;

  MiUnMap<std::string, int> NodeNameToNodeIndex(
      const aiNode* src) const noexcept;
  void ReadHeirarchyData(const aiNode* src, const Skeleton& skeleton) noexcept;

  void CalculateKeyframeBoxes(const Skeleton& skeleton,
                              MiVector<AABB>& keyframe,
                              float current_time) noexcept;
  void CreateAnimationBoxes(const Skeleton& skeleton) noexcept;
};
