#include "object_system.h"

// deps
#include <spdlog/spdlog.h>
// global
#include <numeric>
// local
#include "app/parameters.h"
#include "assets/model_manager.h"

Object::Object(id::Object id, Model &model)
    : id_(id),
      model_(model),
      Props(Props::Flags::kEnabled | Props::Flags::kVisible |
            Props::Flags::kCastShadows) {}

id::Object Object::GetId() const { return id_; }

Model &Object::GetModel() const { return model_; }

const ObjectAddr &Object::GetAddr() const { return addr_; }

bool Object::IsAnimated() const { return addr_.animation_index; }

AnimatedObject::AnimatedObject(id::Object id, Model &model)
    : Object(id, model) {
  animation_ = model_.GetAnimationByIndex(0);
}

Animation *AnimatedObject::GetAnimation() { return animation_; }

void AnimatedObject::SetAnimation(Animation *animation) {
  animation_ = animation;
  time_ = 0.0f;
}

void AnimatedObject::PlayAnimation(std::span<glm::mat4> bone_mat) noexcept {
  // 3.402823E+38, pretty big number
  time_ += animation_->GetTicksPerSecond() * app::glfw.delta_time;
  // always [0.0f, duration]
  time_ = fmod(time_, animation_->GetDuration());
  return animation_->PlayAnimation(model_.GetSkeleton(), time_, bone_mat);
}

gpu::AABB AnimatedObject::GetCurrentBox(GLuint mesh) const {
  return animation_->GetCurrentBox(mesh, time_);
}

GLuint AnimatedObject::GetNumOfBones() const {
  return animation_->GetNumOfBones();
}

ObjectSystem::ObjectSystem(ModelManager &models) noexcept : models_(models) {
  instances_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                        ShaderStorageBinding::kInstances);
  instances_.SetStorage();

  visibility_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                         ShaderStorageBinding::kVisibility);
  visibility_.SetStorage(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                         GL_MAP_COHERENT_BIT);
  visibility_.MapRange(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                       GL_MAP_COHERENT_BIT);

  matrices_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                       ShaderStorageBinding::kMatrices);
  matrices_.SetStorage();

  skinned_boxes_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                            ShaderStorageBinding::kSkinnedBoxes);
  skinned_boxes_.SetStorage();

  animations_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                         ShaderStorageBinding::kAnimations);
  animations_.SetStorage();

  objects_.reserve(1028);
  animated_objects_.reserve(global::kMaxAnimatedActors / 2);
  upload_queue_.reserve(1028);

  upload_bone_mat_.reserve(global::kMaxBoneMatrices / 16);

  upload_matrices_.reserve(1028);
  upload_instances_.reserve(4096);

  // reserve none instance (zero id, disabled)
  gpu::Instance dummy{};
  instances_.AppendArray(&dummy, 1);
}

void ObjectSystem::TrackObject(Object *object) {
  const auto &model = object->GetModel();
  auto &addr = object->addr_;

  addr.instance_count = model.GetInstanceCount();
  addr.first_instance_index = track_instance_;
  addr.matrix_index = track_matrix_;
  addr.first_box_index = model.GetFirstBoxBufferIndex();
  addr.animation_index = 0;

  track_instance_ += addr.instance_count;
  ++track_matrix_;
}

void ObjectSystem::TrackAnimatedObject(Object *object) {
  const auto &model = object->GetModel();
  auto &addr = object->addr_;

  addr.instance_count = model.GetInstanceCount();
  addr.first_instance_index = track_instance_;
  addr.matrix_index = track_matrix_;
  addr.first_box_index = track_skinned_box_;
  addr.animation_index = track_animation_;

  track_instance_ += addr.instance_count;
  ++track_matrix_;
  track_skinned_box_ += addr.instance_count;
  track_animation_ += model.GetNumOfBones();
}

Object *ObjectSystem::CreateObject(const std::string &model_name) {
  Object *object = nullptr;
  Model *model = models_.FindModelMt(model_name);
  if (model == nullptr) {
    spdlog::error("{}: Model is not found '{}'", __FUNCTION__, model_name);
    return object;
  }

  auto id = id::GenId(id::kObject);
  if (model->HasAnimations()) {
    auto [it, res] = animated_objects_.try_emplace(id, id, *model);
    object = &it->second;
    TrackAnimatedObject(object);
  } else {
    auto [it, res] = objects_.try_emplace(id, id, *model);
    object = &it->second;
    TrackObject(object);
  }
  upload_queue_.push_back(object);

  return object;
}

void ObjectSystem::DeleteObject(Object &object) {
  // GPU, overwrite with zeroes
  const auto &addr = object.GetAddr();
  GLintptr offset = sizeof(gpu::Instance) * addr.first_instance_index;
  GLsizeiptr size = sizeof(gpu::Instance) * addr.instance_count;
  glClearNamedBufferSubData(instances_, GL_R32UI, offset, size, GL_RED,
                            GL_UNSIGNED_INT, &global::kZero4Bytes);
  instance_deleted_ += addr.instance_count;
  // CPU
  if (object.IsAnimated()) {
    animated_objects_.erase(object.GetId());
  } else {
    objects_.erase(object.GetId());
  }
}

Object &ObjectSystem::GetObject(id::Object id) {
  auto it = objects_.find(id);
  if (it != objects_.end()) {
    return it->second;
  } else {
    return animated_objects_.at(id);
  }
}

void ObjectSystem::UpdateObject(Object &object) {
  matrices_.UploadIndex(object.GetTransformation(),
                        object.GetAddr().matrix_index);
}

void ObjectSystem::UploadObjectsToGpu() noexcept {
  if (upload_queue_.size() == 0) return;

  upload_instances_.clear();
  upload_matrices_.clear();

  // ProcessAnimation, access via addr
  upload_bone_mat_.resize(track_animation_);
  upload_skinned_boxes_.resize(track_skinned_box_);

  for (const auto *obj : upload_queue_) {
    const auto &meshes = obj->GetModel().meshes_;
    const auto &addr = obj->GetAddr();
    for (GLuint index = 0; const auto &mesh : meshes) {
      GLuint box_index = addr.first_box_index + index;
      upload_instances_.emplace_back(obj->GetId(),                   //
                                     obj->GetProps(),                //
                                     mesh.packed_cmd_buffer_index_,  //
                                     addr.matrix_index,              //
                                     mesh.material_buffer_index_,    //
                                     addr.animation_index,           //
                                     box_index                       //
      );
      ++index;
    }

    upload_matrices_.push_back(obj->GetTransformation());
  }

  instances_.AppendVector(upload_instances_);
  matrices_.AppendVector(upload_matrices_);

  upload_queue_.clear();
}

void ObjectSystem::ProcessAnimations() noexcept {
  for (auto &[id, obj] : animated_objects_) {
    const auto &addr = obj.GetAddr();
    auto first_mat = upload_bone_mat_.begin() + addr.animation_index;
    std::span<glm::mat4> bone_mat{first_mat, obj.GetNumOfBones()};
    obj.PlayAnimation(bone_mat);

    // bounding boxes, precomputed for each keyframe
    for (GLuint i = 0; i < addr.instance_count; ++i) {
      GLuint box_index = addr.first_box_index + i;
      upload_skinned_boxes_[box_index] = obj.GetCurrentBox(i);
    }
  }

  animations_.UploadVector(&gpu::StorageAnimations::bone_matrices,
                           upload_bone_mat_);
  skinned_boxes_.UploadVector(&gpu::StorageSkinnedBoxes::boxes,
                              upload_skinned_boxes_);
}

GLuint ObjectSystem::GetObjectCount() const {
  return static_cast<GLuint>(objects_.size() + animated_objects_.size());
}

GLuint ObjectSystem::GetInstanceCount() const { return track_instance_; }

GLuint ObjectSystem::GetInstanceDeleted() const { return instance_deleted_; }

void ObjectSystem::ReadVisibility() {
  auto it = std::begin(visibility_->instance_visibility);
  for (auto &[id, obj] : objects_) {
    const auto &addr = obj.GetAddr();
    auto begin = it + addr.first_instance_index;
    auto end = it + addr.first_instance_index + addr.instance_count;
    GLuint visibility = std::accumulate(begin, end, 0);
    obj.SetProps(Props::kVisible, visibility);
  }
  for (auto &[id, obj] : animated_objects_) {
    const auto &addr = obj.GetAddr();
    auto begin = it + addr.first_instance_index;
    auto end = it + addr.first_instance_index + addr.instance_count;
    GLuint visibility = std::accumulate(begin, end, 0);
    obj.SetProps(Props::kVisible, visibility);
  }
}

MiUnMap<id::Model, unsigned int> ObjectSystem::CalcObjectsPerModel() noexcept {
  MiUnMap<id::Model, unsigned int> counter;
  counter.reserve(GetObjectCount());
  for (const auto &[id, obj] : objects_) {
    ++counter[obj.GetModel().GetId()];
  }
  for (const auto &[id, obj] : animated_objects_) {
    ++counter[obj.GetModel().GetId()];
  }
  return counter;
}