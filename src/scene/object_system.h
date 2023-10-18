#pragma once

// global
#include <span>
// local
#include "global.h"
#include "id_generator.h"
#include "opengl/buffer_storage.h"
#include "scene/props.h"
// fwd
class ModelManager;
class Model;
class Animation;

namespace gpu {

// Objects can have multiple meshes (GPU instance)
// monotype struct works as array
struct Instance {
  GLuint object_id;
  GLuint props;
  GLuint packed_cmd_index;

  GLuint matrix_index;
  GLuint material_index;
  GLuint animation_index;
  GLuint box_index;
};

// unsorted instances for frustum culling
struct StorageInstances {
  Instance instances[global::kMaxInstances];
};

// Direct Shadowpass: set bits as cascade_index [0, MAX_CASCADES]
// Point Shadowpass: instance_visibility is flat array of cubemap layer indices
// View Pass: boolean visibility
struct StorageVisibility {
  GLuint instance_visibility[global::kMaxInstances];
  glm::uvec2 point_shadows_count_offset[global::kMaxInstances];
};

struct StorageMatrices {
  glm::mat4 world[global::kMaxInstances];
};

// unique boxes per object
// skinned mesh is changed each frame
struct StorageSkinnedBoxes {
  gpu::AABB boxes[global::kMaxInstances];
};

// unique matrices per object
// animation_index works as bone_matrix_offset
struct StorageAnimations {
  glm::mat4 bone_matrices[global::kMaxBoneMatrices];
};

}  // namespace gpu

// allows to generate multiple gpu::Instance from one struct
struct ObjectAddr {
  GLuint instance_count;
  GLuint first_instance_index;
  GLuint matrix_index;
  GLuint first_box_index;
  GLuint animation_index;
};

class Object : public Props, public Transformation {
 public:
  Object(id::Object id, Model& model);

 public:
  // set by System (batch upload to buffer)
  ObjectAddr addr_;

  id::Object GetId() const;
  Model& GetModel() const;
  const ObjectAddr& GetAddr() const;
  bool IsAnimated() const;

 protected:
  id::Object id_;
  // make move-only (resident on GPU)
  Model& model_;
};

class AnimatedObject : public Object {
 public:
  AnimatedObject(id::Object id, Model& model);
  // GPU resource (move-only)
  AnimatedObject(const AnimatedObject&) = delete;
  AnimatedObject& operator=(const AnimatedObject&) = delete;
  AnimatedObject(AnimatedObject&&) = default;
  AnimatedObject& operator=(AnimatedObject&&) = default;

 public:
  Animation* GetAnimation();
  void SetAnimation(Animation* animation);
  void PlayAnimation(std::span<glm::mat4> bone_mat) noexcept;
  GLuint GetNumOfBones() const;
  gpu::AABB GetCurrentBox(GLuint mesh) const;

 private:
  Animation* animation_{nullptr};
  float time_{0.0f};
};

class ObjectSystem {
 public:
  ObjectSystem(ModelManager& models) noexcept;
  using Inject = ObjectSystem(ModelManager&);

 public:
  Object* CreateObject(const std::string& model_name);
  void DeleteObject(Object& object);
  Object& GetObject(id::Object id);
  void UpdateObject(Object& object);

  void ProcessAnimations() noexcept;
  void UploadObjectsToGpu() noexcept;

  GLuint GetObjectCount() const;
  // GPU count (with deleted)
  GLuint GetInstanceCount() const;
  GLuint GetInstanceDeleted() const;

  void ReadVisibility();

  MiUnMap<id::Model, unsigned int> CalcObjectsPerModel() noexcept;

 private:
  ModelManager& models_;

  gl::CountedBuffer<gpu::StorageInstances> instances_{"Instances"};
  gl::MappedBuffer<gpu::StorageVisibility> visibility_{"Visibility"};
  gl::CountedBuffer<gpu::StorageMatrices> matrices_{"Matrices"};
  gl::StreamBuffer<gpu::StorageSkinnedBoxes> skinned_boxes_{"SkinnedBoxes"};
  gl::StreamBuffer<gpu::StorageAnimations> animations_{"Animations"};

  // track positions inside GPU's buffers
  // first instance reserved for none (like nullptr)
  GLuint track_instance_{1};
  GLuint track_matrix_{0};
  GLuint track_skinned_box_{0};
  // animation_index == 0 for Static
  GLuint track_animation_{1};

  GLuint instance_deleted_{0};

  void TrackObject(Object* object);
  void TrackAnimatedObject(Object* object);

  MiUnMap<id::Object, Object> objects_;
  MiUnMap<id::Object, AnimatedObject> animated_objects_;
  // Setup stage changes data, delay upload
  MiVector<Object*> upload_queue_;

  MiVector<glm::mat4> upload_matrices_;
  MiVector<gpu::Instance> upload_instances_;

  // animations
  MiVector<glm::mat4> upload_bone_mat_;
  MiVector<gpu::AABB> upload_skinned_boxes_;
};