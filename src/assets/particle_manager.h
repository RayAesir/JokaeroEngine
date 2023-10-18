#pragma once

// local
#include "assets/texture_manager.h"
#include "events.h"
#include "global.h"
#include "math/collision_types.h"
#include "opengl/buffer_storage.h"
// fwd
namespace ui {
class WinResources;
}

namespace gpu {

struct FxPreset {
  glm::mat4 emitter_basis;
  glm::vec4 acceleration;
  glm::vec4 direction_constraints;
  glm::vec2 start_position_min_max;
  glm::vec2 start_velocity_min_max;
  //
  GLfloat particle_lifetime;
  GLfloat cone_angle_rad;
  GLuint num_of_particles;
  GLuint buffer_offset;
};

struct StorageFxPresets {
  FxPreset fx_presets[global::kMaxFxPresets];
  glm::vec4 positions[global::kMaxParticlesBufferSize];
  glm::vec4 velocities[global::kMaxParticlesBufferSize];
  GLfloat ages[global::kMaxParticlesBufferSize];
};

}  // namespace gpu

struct FxPreset {
  std::string name;
  GLint buffer_index;
  GLuint buffer_offset;
  std::shared_ptr<SmartTexture> texture;
  AABB box;
  // base
  glm::vec3 emitter_dir;
  glm::vec3 acceleration;
  glm::vec3 direction_constraints;
  glm::vec2 start_position_min_max;
  glm::vec2 start_velocity_min_max;
  float particle_lifetime;
  float cone_angle_deg;
  unsigned int num_of_particles;
  // can be changed per instance
  glm::vec2 particle_size_min_max;
  glm::vec3 color;
  bool should_fade_out;
};

class ParticleManager : public event::Base<ParticleManager> {
 public:
  ParticleManager(TextureManager &textures, ui::WinResources &ui);
  using Inject = ParticleManager(TextureManager &, ui::WinResources &);

 public:
  FxPreset *FindFxPresetByName(const std::string &name);

 private:
  TextureManager &textures_;
  ui::WinResources &ui_;

  gl::StreamBuffer<gpu::StorageFxPresets> fx_presets_{"FxPresets"};

  // presets
  GLint tracker_preset_;
  MiVector<FxPreset> fx_;

  void CreateDefaultFXs();
  GLuint CalculateBufferOffsets();
  void CalculateBoxes(const MiVector<gpu::FxPreset> &upload_fx_gpu) noexcept;
  void InitFX() noexcept;

  void InitEvents();
};