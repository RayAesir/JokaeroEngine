#include "particle_manager.h"

// local
#include "files.h"
#include "math/random.h"
#include "ui/win_resources.h"

ParticleManager::ParticleManager(TextureManager &textures, ui::WinResources &ui)
    : event::Base<ParticleManager>(&ParticleManager::InitEvents, this),
      textures_(textures),
      ui_(ui) {
  fx_presets_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                         ShaderStorageBinding::kFxPresets);
  fx_presets_.SetStorage();

  CreateDefaultFXs();
  InitFX();
}

void ParticleManager::InitEvents() {
  Connect(Event::kUpdateFx, [this]() { InitFX(); });
  Connect(Event::kResetFx, [this]() {
    fx_.clear();
    ui_.list_fx_.Reset();
    CreateDefaultFXs();
    InitFX();
  });
}

void ParticleManager::CreateDefaultFXs() {
  tracker_preset_ = 0;
  fx_.push_back({
      .name = "Fire",
      .buffer_index = tracker_preset_++,
      .buffer_offset = 0,
      .texture = textures_.CreateTextureMt(
          files::particles.str + "/fire_01.png", TextureType::kDiffuse),
      //
      .emitter_dir = glm::vec3(0.0f, 1.0f, 0.0f),
      .acceleration = glm::vec3(0.0f, 0.1f, 0.0f),
      .direction_constraints = glm::vec3(0.0f, 1.0f, 0.0f),
      .start_position_min_max = glm::vec2(-2.0f, 2.0f),
      .start_velocity_min_max = glm::vec2(0.1f, 0.5f),
      .particle_lifetime = 3.0f,
      .cone_angle_deg = 0.0f,
      .num_of_particles = 1200,
      //
      .particle_size_min_max = glm::vec2(0.5f),
      .color = glm::vec3(0.882f, 0.343f, 0.132f),
      .should_fade_out = true,
  });

  fx_.push_back({
      .name = "Smoke",
      .buffer_index = tracker_preset_++,
      .buffer_offset = 0,
      .texture = textures_.CreateTextureMt(
          files::particles.str + "/smoke_01.png", TextureType::kDiffuse),
      //
      .emitter_dir = glm::vec3(0.0f, 1.0f, 0.0f),
      .acceleration = glm::vec3(0.0f, 0.1f, 0.0f),
      .direction_constraints = glm::vec3(1.0f),
      .start_position_min_max = glm::vec2(0.0f),
      .start_velocity_min_max = glm::vec2(0.1f, 0.2f),
      .particle_lifetime = 10.0f,
      .cone_angle_deg = glm::degrees(glm::pi<float>() / 1.5f),
      .num_of_particles = 1200,
      //
      .particle_size_min_max = glm::vec2(0.1f, 2.5f),
      .color = glm::vec3(0.961f, 0.961f, 0.961f),
      .should_fade_out = false,
  });

  fx_.push_back({
      .name = "Magic",
      .buffer_index = tracker_preset_++,
      .buffer_offset = 0,
      .texture = textures_.CreateTextureMt(
          files::particles.str + "/star_01.png", TextureType::kDiffuse),
      //
      .emitter_dir = glm::vec3(0.0f, 1.0f, 0.0f),
      .acceleration = glm::vec3(0.0f, -0.5f, 0.0f),
      .direction_constraints = glm::vec3(1.0f),
      .start_position_min_max = glm::vec2(0.0f),
      .start_velocity_min_max = glm::vec2(1.25f, 1.5f),
      .particle_lifetime = 10.0f,
      .cone_angle_deg = glm::degrees(glm::pi<float>() / 8.0f),
      .num_of_particles = 1200,
      //
      .particle_size_min_max = glm::vec2(0.3f),
      .color = glm::vec3(0.15f, 0.24f, 0.65f),
      .should_fade_out = false,
  });

  ui_.list_fx_.label = "FX Presets";
  ui_.list_fx_.current = 0;
  for (auto &fx : fx_) {
    ui_.list_fx_.EmplaceBack(&fx, fx.name);
  }
}

GLuint ParticleManager::CalculateBufferOffsets() {
  GLuint total = 0;
  for (auto &fx : fx_) {
    fx.buffer_offset = total;
    total += fx.num_of_particles;
  }
  return total;
}

glm::vec3 RandomInitialVelocity(const gpu::FxPreset &fx,
                                const glm::vec3 &random) {
  float theta = glm::mix(0.0f, fx.cone_angle_rad, random.x);
  float phi = glm::mix(0.0f, glm::two_pi<float>(), random.y);
  float velocity = glm::mix(fx.start_velocity_min_max.x,
                            fx.start_velocity_min_max.y, random.z);
  glm::vec3 v =
      glm::vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

  const glm::mat3 emitter_basis = fx.emitter_basis;
  const glm::vec3 &direction_constraints = fx.direction_constraints;
  return glm::normalize(emitter_basis * v * direction_constraints) * velocity;
}

glm::vec3 RandomInitialPosition(const gpu::FxPreset &fx, float random) {
  float offset = glm::mix(fx.start_position_min_max.x,
                          fx.start_position_min_max.y, random);
  return glm::vec3(offset, 0.0f, 0.0f);
}

void ParticleManager::CalculateBoxes(
    const MiVector<gpu::FxPreset> &upload_fx_gpu) noexcept {
  for (size_t i = 0; i < fx_.size(); ++i) {
    FxPreset &preset = fx_[i];
    const gpu::FxPreset &fx = upload_fx_gpu[i];
    MiVector<glm::vec3> points;
    // start position
    glm::vec3 pos_min = RandomInitialPosition(fx, 0.0f);
    glm::vec3 pos_max = RandomInitialPosition(fx, 1.0f);
    points.push_back(pos_min);
    points.push_back(pos_max);
    // all possible extremums during lifetime
    for (float pos = 0.0f; pos <= 1.0f; pos += 1.0f) {
      for (float x = 0.0f; x <= 1.0f; x += 1.0f) {
        for (float y = 0.0f; y <= 1.0f; y += 0.25f) {
          for (float z = 0.0f; z <= 1.0f; z += 1.0f) {
            glm::vec3 velocity = RandomInitialVelocity(fx, glm::vec3(x, y, z));
            glm::vec3 position = RandomInitialPosition(fx, pos);
            // two points per second should be enough for most cases
            constexpr float t = 0.5f;
            for (float step = 0.0f; step <= preset.particle_lifetime;
                 step += t) {
              position = position + (velocity * t) +
                         (0.5f * preset.acceleration * t * t);
              velocity = velocity + (preset.acceleration * t);
              points.push_back(position);
            }
          }
        }
      }
    }
    preset.box = AABB{points};
  }
}

glm::mat3 MakeArbitraryBasis(const glm::vec3 &dir) {
  glm::mat3 basis;
  glm::vec3 u, v, n;
  v = dir;
  n = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), v);
  if (glm::length(n) < 0.00001f) {
    n = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), v);
  }
  u = glm::cross(v, n);
  basis[0] = glm::normalize(u);
  basis[1] = glm::normalize(v);
  basis[2] = glm::normalize(n);
  return basis;
}

void ParticleManager::InitFX() noexcept {
  // buffers, prepare one big array for all FXs
  GLuint total_particles = CalculateBufferOffsets();
  MiVector<GLfloat> initial_particle_ages;
  initial_particle_ages.reserve(total_particles);
  for (const auto &fx : fx_) {
    float particle_lifetime = fx.particle_lifetime;
    int num_of_particles = static_cast<int>(fx.num_of_particles);
    float rate = particle_lifetime / num_of_particles;
    for (int i = 0; i < num_of_particles; ++i) {
      initial_particle_ages.push_back(rate * (i - num_of_particles));
    }
  }
  for (const auto &fx : fx_) {
    std::shuffle(
        initial_particle_ages.begin() + fx.buffer_offset,
        initial_particle_ages.begin() + fx.buffer_offset + fx.num_of_particles,
        rnd::GetPRNG());
  }

  // convert to GPU format (std430 layout)
  MiVector<gpu::FxPreset> upload_fx_gpu;
  upload_fx_gpu.reserve(fx_.size());
  for (const auto &fx : fx_) {
    upload_fx_gpu.emplace_back(MakeArbitraryBasis(fx.emitter_dir),         //
                               glm::vec4(fx.acceleration, 0.0f),           //
                               glm::vec4(fx.direction_constraints, 0.0f),  //
                               fx.start_position_min_max,                  //
                               fx.start_velocity_min_max,                  //
                                                                           //
                               fx.particle_lifetime,                       //
                               glm::radians(fx.cone_angle_deg),            //
                               fx.num_of_particles,                        //
                               fx.buffer_offset                            //
    );
  }

  // frustum culling
  CalculateBoxes(upload_fx_gpu);

  // upload
  glClearNamedBufferData(fx_presets_, GL_R32UI, GL_RED, GL_UNSIGNED_INT,
                         &global::kZero4Bytes);
  fx_presets_.UploadVector(&gpu::StorageFxPresets::fx_presets, upload_fx_gpu);
  fx_presets_.UploadVector(&gpu::StorageFxPresets::ages, initial_particle_ages);
}

FxPreset *ParticleManager::FindFxPresetByName(const std::string &name) {
  auto it = std::find_if(fx_.begin(), fx_.end(), [&name](const FxPreset &fx) {
    return fx.name == name;
  });
  if (it == fx_.end()) {
    spdlog::warn("{}: FX not found '{}'", __FUNCTION__, name);
    return nullptr;
  }
  return &(*it);
}
