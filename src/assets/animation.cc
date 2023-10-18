#include "animation.h"

// deps
#include <assimp/scene.h>
// local
#include "math/assimp_to_glm.h"

Bone::Bone(int id, const aiNodeAnim* channel) noexcept
    : id_(id), local_transform_(1.0f) {
  num_positions = channel->mNumPositionKeys;
  positions.reserve(num_positions);
  positions_time.reserve(num_positions);
  for (unsigned int i = 0; i < num_positions; ++i) {
    positions.emplace_back(assglm::GetVec(channel->mPositionKeys[i].mValue));
    positions_time.emplace_back(
        static_cast<float>(channel->mPositionKeys[i].mTime));
  }

  num_rotations = channel->mNumRotationKeys;
  rotations.reserve(num_rotations);
  rotations_time.reserve(num_rotations);
  for (unsigned int i = 0; i < num_rotations; ++i) {
    rotations.emplace_back(assglm::GetQuat(channel->mRotationKeys[i].mValue));
    rotations_time.emplace_back(
        static_cast<float>(channel->mRotationKeys[i].mTime));
  }

  num_scalings = channel->mNumScalingKeys;
  scales.reserve(num_scalings);
  scales_time.reserve(num_scalings);
  for (unsigned int i = 0; i < num_scalings; ++i) {
    scales.emplace_back(assglm::GetVec(channel->mScalingKeys[i].mValue));
    scales_time.emplace_back(
        static_cast<float>(channel->mScalingKeys[i].mTime));
  }
};

void Bone::Update(float animation_time) {
  glm::vec3 translation = InterpolatePosition(animation_time);
  glm::quat rotation = InterpolateRotation(animation_time);
  glm::vec3 scale = InterpolateScaling(animation_time);

  local_transform_ = glm::mat4_cast(rotation);
  local_transform_[0] = scale.x * local_transform_[0];
  local_transform_[1] = scale.y * local_transform_[1];
  local_transform_[2] = scale.z * local_transform_[2];
  local_transform_[3].x = translation.x;
  local_transform_[3].y = translation.y;
  local_transform_[3].z = translation.z;
  local_transform_[3].w = 1.0f;
}

const glm::mat4& Bone::GetLocalTransform() const { return local_transform_; }

int Bone::GetBoneID() const { return id_; }

const int Bone::GetPositionIndex(float animation_time) {
  for (unsigned int i = 0; i < num_positions - 1; ++i) {
    if (animation_time < positions_time[i + 1]) {
      return i;
    }
  }
  return 0;
}

const int Bone::GetRotationIndex(float animation_time) {
  for (unsigned int i = 0; i < num_rotations - 1; ++i) {
    if (animation_time < rotations_time[i + 1]) {
      return i;
    }
  }
  return 0;
}

const int Bone::GetScaleIndex(float animation_time) {
  for (unsigned int i = 0; i < num_scalings - 1; ++i) {
    if (animation_time < scales_time[i + 1]) {
      return i;
    }
  }
  return 0;
}

float Bone::GetScaleFactor(float last_timestamp, float next_timestamp,
                           float animation_time) {
  float scale_factor = 0.0f;
  float midway_length = animation_time - last_timestamp;
  float frames_diff = next_timestamp - last_timestamp;
  scale_factor = midway_length / frames_diff;
  return scale_factor;
}

glm::vec3 Bone::InterpolatePosition(float animation_time) {
  if (num_positions == 1) return positions[0];

  int p0 = GetPositionIndex(animation_time);
  int p1 = p0 + 1;
  float scale_factor =
      GetScaleFactor(positions_time[p0], positions_time[p1], animation_time);
  glm::vec3 final_position =
      glm::mix(positions[p0], positions[p1], scale_factor);
  return final_position;
}

glm::quat Bone::InterpolateRotation(float animation_time) {
  if (num_rotations == 1) {
    glm::quat rotation = glm::normalize(rotations[0]);
    return rotation;
  }

  int p0 = GetRotationIndex(animation_time);
  int p1 = p0 + 1;
  float scale_factor =
      GetScaleFactor(rotations_time[p0], rotations_time[p1], animation_time);
  glm::quat final_rotation =
      glm::slerp(rotations[p0], rotations[p1], scale_factor);
  final_rotation = glm::normalize(final_rotation);
  return final_rotation;
}

glm::vec3 Bone::InterpolateScaling(float animation_time) {
  if (num_scalings == 1) return scales[0];

  int p0 = GetScaleIndex(animation_time);
  int p1 = p0 + 1;
  float scale_factor =
      GetScaleFactor(scales_time[p0], scales_time[p1], animation_time);
  glm::vec3 final_scale = glm::mix(scales[p0], scales[p1], scale_factor);
  return final_scale;
}

Animation::Animation(aiAnimation* animation, aiNode* root_node,
                     Skeleton& skeleton) noexcept
    : name_(animation->mName.C_Str()) {
  duration_ = animation->mDuration;
  ticks_per_second_ = animation->mTicksPerSecond;
  // 1. create bones and gather matrices
  ReadBonesData(animation, skeleton);
  // 2. read node hierarchy
  ReadHeirarchyData(root_node, skeleton);
  // 3. create boxes
  CreateAnimationBoxes(skeleton);
}

const std::string& Animation::GetName() const { return name_; }

float Animation::GetTicksPerSecond() const { return ticks_per_second_; }

float Animation::GetDuration() const { return duration_; }

const MiVector<Bone>& Animation::GetBones() const { return bones_; }

GLuint Animation::GetNumOfBones() const {
  return static_cast<GLuint>(bones_.size());
}

gpu::AABB Animation::GetCurrentBox(GLuint mesh, float time) const {
  auto& keyframes = boxes_[mesh];
  float whole;
  float frac;
  frac = std::modf(time, &whole);
  // convert
  size_t current_tick = static_cast<size_t>(whole);
  size_t next_tick = current_tick + 1;
  if (keyframes.size() == next_tick) {
    // last frame
    return keyframes[current_tick];
  } else {
    // interpolate boxes
    const glm::vec3& c1 = keyframes[current_tick].center;
    const glm::vec3& e1 = keyframes[current_tick].extent;
    const glm::vec3& c2 = keyframes[next_tick].center;
    const glm::vec3& e2 = keyframes[next_tick].extent;
    glm::vec3 c = glm::mix(c1, c2, frac);
    glm::vec3 e = glm::mix(e1, e2, frac);
    return gpu::AABB{c, e};
  }
}

void Animation::ReadBonesData(const aiAnimation* animation,
                              Skeleton& skeleton) noexcept {
  auto& bone_map = skeleton.bone_map;
  auto& bone_to_local = skeleton.bone_to_local;
  auto& bone_box = skeleton.bone_box;
  MiUnMap<int, aiNodeAnim*> channels;

  // reading channels (bones engaged in an animation and their keyframes)
  bones_.reserve(animation->mNumChannels);
  channels.reserve(animation->mNumChannels);
  for (unsigned int i = 0; i < animation->mNumChannels; i++) {
    aiNodeAnim* channel = animation->mChannels[i];
    std::string bone_name = channel->mNodeName.C_Str();

    // add missing bones
    // Blender exports Armature as channel
    if (bone_map.contains(bone_name) == false) {
      int bone_count = static_cast<int>(bone_map.size());
      bone_map.try_emplace(bone_name, bone_count);
      bone_to_local.emplace_back(glm::mat4(1.0f));
      // default empty box
      bone_box.try_emplace(bone_count);
    }

    int bone_id = bone_map.at(bone_name);
    channels.try_emplace(bone_id, channel);
  }

  // convert map to vector
  for (int i = 0; i < static_cast<int>(channels.size()); ++i) {
    bones_.emplace_back(i, channels.at(i));
  }
}

// all nodes in the scene: bones, meshes, armatures...
MiUnMap<std::string, int> Animation::NodeNameToNodeIndex(
    const aiNode* src) const noexcept {
  MiVector<const aiNode*> stack;
  stack.push_back(src);

  MiUnMap<std::string, int> map;
  map.reserve(bones_.size() + src->mNumMeshes);

  int index = 0;
  while (!stack.empty()) {
    const aiNode* top = stack.back();
    stack.pop_back();

    // traverse order as index inside vector
    map.try_emplace(top->mName.C_Str(), index++);

    for (unsigned int i = 0; i < top->mNumChildren; ++i) {
      stack.push_back(top->mChildren[i]);
    }
  }

  return map;
}

// DFS = Depth-First Search to traverse graph
void Animation::ReadHeirarchyData(const aiNode* src,
                                  const Skeleton& skeleton) noexcept {
  MiVector<const aiNode*> stack;
  stack.push_back(src);

  const auto& bone_map = skeleton.bone_map;
  auto name_to_node_id = NodeNameToNodeIndex(src);
  nodes_data_.resize(name_to_node_id.size());

  int index = 0;
  while (!stack.empty()) {
    const aiNode* top = stack.back();
    stack.pop_back();

    NodeData& node = nodes_data_[index++];
    std::string node_name{top->mName.C_Str()};
    node.children.reserve(top->mNumChildren);
    if (auto it = bone_map.find(node_name); it != bone_map.end()) {
      node.bone_id = it->second;
    } else {
      node.transformation = assglm::GetMatrix(top->mTransformation);
    }

    for (unsigned int i = 0; i < top->mNumChildren; ++i) {
      stack.push_back(top->mChildren[i]);
      int child_index = name_to_node_id.at(top->mChildren[i]->mName.C_Str());
      node.children.push_back(child_index);
    }
  }
}

void Animation::CalculateKeyframeBoxes(const Skeleton& skeleton,
                                       MiVector<AABB>& keyframe,
                                       float current_time) noexcept {
  MiVector<NodeData*> stack;
  MiVector<glm::mat4> parent;
  stack.push_back(&nodes_data_[0]);
  parent.push_back(glm::mat4(1.0f));

  while (!stack.empty()) {
    const NodeData* node = stack.back();
    const glm::mat4 parent_transform = parent.back();
    stack.pop_back();
    parent.pop_back();

    // process
    int bone_id = node->bone_id;
    glm::mat4 node_transform;
    glm::mat4 global_transformation;

    if (bone_id >= 0) {
      // local
      auto& bone = bones_[bone_id];
      bone.Update(current_time);
      node_transform = bone.GetLocalTransform();
      // bone chain
      global_transformation = parent_transform * node_transform;
      // mesh space
      const auto& offset = skeleton.bone_to_local[bone_id];
      const auto& bone_box = skeleton.bone_box.at(bone_id);
      // box
      glm::mat4 t{global_transformation * offset};
      auto& box = keyframe[bone_id];
      box.Transform(bone_box, t);
    } else {
      // other scene objects e.g. meshes
      node_transform = node->transformation;
      global_transformation = parent_transform * node_transform;
    }

    for (const auto child : node->children) {
      stack.push_back(&nodes_data_[child]);
      parent.push_back(global_transformation);
    }
  }
}

void Animation::CreateAnimationBoxes(const Skeleton& skeleton) noexcept {
  const auto& bones_per_model = skeleton.bones_per_model;
  boxes_.resize(bones_per_model.size());
  for (size_t mesh = 0; mesh < bones_per_model.size(); ++mesh) {
    boxes_[mesh].reserve(static_cast<size_t>(duration_));
  }

  // process every animation's tick
  MiVector<AABB> keyframe;
  for (float tick = 0.0f; tick < duration_; ++tick) {
    // reuse vector
    keyframe.clear();
    keyframe.resize(bones_.size());

    CalculateKeyframeBoxes(skeleton, keyframe, tick);

    // make the single mesh box that encapsulated all bone's boxes
    for (size_t mesh = 0; mesh < bones_per_model.size(); ++mesh) {
      AABB mesh_box;
      for (const auto bone : bones_per_model[mesh]) {
        mesh_box.ExpandToInclude(keyframe[bone]);
      }
      boxes_[mesh].emplace_back(mesh_box.center_, mesh_box.extent_);
    }
  }
}

void Animation::PlayAnimation(const Skeleton& skeleton, float time,
                              std::span<glm::mat4> bone_mat) noexcept {
  MiVector<NodeData*> stack;
  MiVector<glm::mat4> parent;
  stack.push_back(&nodes_data_[0]);
  parent.push_back(glm::mat4(1.0f));

  while (!stack.empty()) {
    const NodeData* node = stack.back();
    const glm::mat4 parent_transform = parent.back();
    stack.pop_back();
    parent.pop_back();

    // process
    int bone_id = node->bone_id;
    glm::mat4 node_transform;
    glm::mat4 global_transformation;

    if (bone_id >= 0) {
      // local
      Bone& bone = bones_[bone_id];
      bone.Update(time);
      node_transform = bone.GetLocalTransform();
      // bone chain
      global_transformation = parent_transform * node_transform;
      // mesh space
      const auto& offset = skeleton.bone_to_local[bone_id];
      bone_mat[bone_id] = global_transformation * offset;
    } else {
      // other scene objects e.g. meshes
      node_transform = node->transformation;
      global_transformation = parent_transform * node_transform;
    }

    for (const auto child : node->children) {
      stack.push_back(&nodes_data_[child]);
      parent.push_back(global_transformation);
    }
  }
}
