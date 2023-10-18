#pragma once

// global
#include <unordered_set>
// local
#include "events.h"
#include "global.h"
#include "id_generator.h"
#include "math/collision_types.h"
#include "opengl/buffer_storage.h"
// fwd
class ParticleManager;
class MainCamera;
struct FxPreset;

namespace gpu {

// default OpenGL CMD
struct DrawArraysIndirectCommand {
  GLuint count;
  GLuint instance_count;
  GLuint first_index;
  GLuint base_instance;
};

struct IndirectParticles {
  DrawArraysIndirectCommand commands[global::kMaxFxPresets];
};

// duplicate particle_lifetime to avoid access to StorageFxs
struct FxInstance {
  glm::vec2 particle_size_min_max;
  GLfloat particle_lifetime;
  GLfloat should_keep_color;
  //
  glm::vec4 color;
  glm::vec4 emitter_pos;
  //
  GLuint64 tex_handler;
  GLuint64 empty_pad0;
};

struct StorageFxInstances {
  FxInstance fx_instances[global::kMaxFxInstances];
};

struct StorageFxBoxes {
  AABB fx_boxes[global::kMaxFxInstances];
};

}  // namespace gpu

struct FxInstance {
  id::FxInstance id;
  const FxPreset *fx;
  gpu::FxInstance particles;
};

template <typename T>
using MiUnSet =
    std::unordered_set<T, std::hash<T>, std::equal_to<T>, mi_stl_allocator<T>>;

class ParticleSystem : public event::Base<ParticleSystem> {
 public:
  ParticleSystem(ParticleManager &particles, MainCamera &camera) noexcept;
  using Inject = ParticleSystem(ParticleManager &, MainCamera &);

 public:
  GLuint fx_instance_count_{0};

  FxInstance *ProcessParticles() noexcept;
  const MiUnSet<const FxPreset *> &GetFxToDraw() const;
  void DrawParticles() const;

  void CreateParticles(const std::string &name, glm::vec3 position);
  void CreateCustomParticles(const std::string &name, glm::vec3 position,
                             glm::vec3 color, glm::vec2 size_min_max,
                             bool fade_out);
  void DeleteParticles(FxInstance *ptr);
  void DeleteParticles(id::FxInstance id);

  void UpdateParticlesFromPreset(FxInstance &instance);

 private:
  ParticleManager &particles_;
  MainCamera &camera_;

  gl::StreamBuffer<gpu::IndirectParticles> indirect_particles_{
      "IndirectParticles"};
  gl::StreamBuffer<gpu::StorageFxInstances> fx_instances_{"FxInstances"};
  gl::StreamBuffer<gpu::StorageFxBoxes> fx_boxes_{"FxBoxes"};

  MiUnMap<id::FxInstance, FxInstance> instances_;
  // presets
  MiVector<gpu::DrawArraysIndirectCommand> draw_cmd_;
  // instances
  MiUnSet<const FxPreset *> fx_to_draw_;
  MiVector<gpu::FxInstance> upload_fx_instances_;
  MiVector<gpu::AABB> upload_fx_boxes_;

  void InitEvents();
};