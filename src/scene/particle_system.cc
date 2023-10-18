#include "particle_system.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "assets/particle_manager.h"
#include "math/intersection.h"
#include "scene/camera_system.h"

ParticleSystem::ParticleSystem(ParticleManager &particles,
                               MainCamera &camera) noexcept
    : event::Base<ParticleSystem>(&ParticleSystem::InitEvents, this),
      particles_(particles),
      camera_(camera) {
  indirect_particles_.SetStorage();

  fx_instances_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                           ShaderStorageBinding::kFxInstances);
  fx_instances_.SetStorage();

  fx_boxes_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                       ShaderStorageBinding::kFxBoxes);
  fx_boxes_.SetStorage();

  draw_cmd_.reserve(global::kMaxFxPresets);
  fx_to_draw_.reserve(global::kMaxFxPresets);
  upload_fx_instances_.reserve(global::kMaxFxInstances);
  upload_fx_boxes_.reserve(global::kMaxFxInstances);
  instances_.reserve(16);
}

void ParticleSystem::InitEvents() {
  Connect(Event::kUpdateParticlesInstances, [this]() {
    for (auto &[id, instance] : instances_) {
      UpdateParticlesFromPreset(instance);
    }
  });
}

FxInstance *ParticleSystem::ProcessParticles() noexcept {
  fx_to_draw_.clear();
  draw_cmd_.clear();
  upload_fx_instances_.clear();
  upload_fx_boxes_.clear();

  FxInstance *look_at = nullptr;
  float closest = 0.0f;
  for (auto &[id, instance] : instances_) {
    const FxPreset *fx = instance.fx;
    const gpu::FxInstance &particles = instance.particles;
    fx_to_draw_.insert(fx);

    // move box, full transformation excessive
    AABB box = fx->box;
    const glm::vec3 &pos = particles.emitter_pos;
    const glm::vec3 expand{particles.particle_size_min_max.y * 0.5f};
    box.MoveAndResize(pos, expand);

    if (math::AABBInFrustum(box, camera_.frustum_)) {
      // two triangles as quad
      draw_cmd_.emplace_back(6,                     // count
                             fx->num_of_particles,  // instance_count
                             0,                     // first_index
                             fx->buffer_offset      // base_instance
      );
      upload_fx_instances_.push_back(particles);
      upload_fx_boxes_.emplace_back(box.center_, box.extent_);

      // look at particles
      float dist = 0.0f;
      if (math::RayAABB(box, camera_.ray_, dist)) {
        glm::vec3 dir_to_box =
            glm::normalize(box.center_ - camera_.ray_.origin_);
        float LdotR = glm::dot(dir_to_box, camera_.ray_.dir_);
        // drop boxes behind camera
        if (LdotR > 0.0f) {
          // the bigger LdotR, the closer crosshair to box's center
          if (LdotR >= closest) {
            look_at = &instance;
            closest = LdotR;
          }
        }
      }
    }
  }
  fx_instance_count_ = static_cast<GLuint>(upload_fx_instances_.size());

  indirect_particles_.UploadVector(&gpu::IndirectParticles::commands,
                                   draw_cmd_);
  fx_instances_.UploadVector(&gpu::StorageFxInstances::fx_instances,
                             upload_fx_instances_);
  fx_boxes_.UploadVector(&gpu::StorageFxBoxes::fx_boxes, upload_fx_boxes_);

  return look_at;
}

const MiUnSet<const FxPreset *> &ParticleSystem::GetFxToDraw() const {
  return fx_to_draw_;
}

void ParticleSystem::DrawParticles() const {
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_particles_);
  glMultiDrawArraysIndirect(GL_TRIANGLES, reinterpret_cast<GLvoid *>(0),
                            static_cast<GLsizei>(draw_cmd_.size()), 0);
}

void ParticleSystem::CreateParticles(const std::string &name,
                                     glm::vec3 position) {
  FxPreset *fx = particles_.FindFxPresetByName(name);
  if (fx) {
    auto id = id::GenId(id::kFxInstance);
    instances_.try_emplace(
        id, id, fx,
        gpu::FxInstance{
            fx->particle_size_min_max,                   //
            fx->particle_lifetime,                       //
            static_cast<GLfloat>(!fx->should_fade_out),  //
            glm::vec4(fx->color, 1.0f),                  //
            // compute shader outputs positions as vec4(x, y, z, 1.0), here 0.0
            glm::vec4(position, 0.0f),  //
            fx->texture->GetHandler(),  //
            static_cast<GLuint64>(0)    //
        });
  }
}

void ParticleSystem::CreateCustomParticles(const std::string &name,
                                           glm::vec3 position, glm::vec3 color,
                                           glm::vec2 size_min_max,
                                           bool fade_out) {
  FxPreset *fx = particles_.FindFxPresetByName(name);
  if (fx) {
    auto id = id::GenId(id::kFxInstance);
    instances_.try_emplace(id, id, fx,
                           gpu::FxInstance{
                               size_min_max,                     //
                               fx->particle_lifetime,            //
                               static_cast<GLfloat>(!fade_out),  //
                               glm::vec4(color, 1.0f),           //
                               // compute shader outputs positions as
                               // vec4(x, y, z, 1.0), here 0.0
                               glm::vec4(position, 0.0f),  //
                               fx->texture->GetHandler(),  //
                               static_cast<GLuint64>(0)    //
                           });
  }
}

void ParticleSystem::DeleteParticles(FxInstance *ptr) {
  instances_.erase(ptr->id);
}

void ParticleSystem::DeleteParticles(id::FxInstance id) {
  instances_.erase(id);
}

void ParticleSystem::UpdateParticlesFromPreset(FxInstance &instance) {
  const auto *fx = instance.fx;
  auto &particles = instance.particles;
  particles.particle_size_min_max = fx->particle_size_min_max;
  particles.particle_lifetime = fx->particle_lifetime;
  particles.should_keep_color = !fx->should_fade_out;
  particles.color = glm::vec4(fx->color, 1.0f);
  particles.tex_handler = fx->texture->GetHandler();
}