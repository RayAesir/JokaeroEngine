#include "light_system.h"

PointLight::PointLight(id::PointLight id, bool cast_shadows)
    : id_(id), Props(Props::Flags::kEnabled | Props::Flags::kVisible) {
  SetProps(Props::kCastShadows, cast_shadows);
}

id::PointLight PointLight::GetId() const { return id_; }

Sphere PointLight::GetSphere() const noexcept {
  return Sphere{position_, radius_};
}

gpu::PointLight PointLight::ConvertToGpu() const noexcept {
  return gpu::PointLight{
      .id = id_,        //
      .empty_pad0 = 0,  //
      .empty_pad1 = 0,  //
      .empty_pad2 = 0,  //
      //
      .props = GetProps(),                        //
      .radius = radius_,                          //
      .radius2 = radius_ * radius_,               //
      .inv_radius2 = 1.0f / (radius_ * radius_),  //
      //
      .diffuse = glm::vec4(diffuse_, intensity_),  //
      .position = glm::vec4(position_, 1.0f)       //
  };
}

LightSystem::LightSystem() noexcept {
  gpu_point_lights_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                               ShaderStorageBinding::kPointLights);
  gpu_point_lights_.SetStorage();

  point_lights_.reserve(global::kMaxPointLights / 2);
  upload_queue_.reserve(global::kMaxPointLights / 2);
  upload_point_lights_.reserve(global::kMaxPointLights / 2);
  // reserve none instance (zero id, disabled)
  gpu::PointLight dummy{};
  gpu_point_lights_.AppendArray(&dummy, 1);
}

PointLight &LightSystem::CreatePointLight(bool cast_shadows) {
  auto id = id::GenId(id::kPointLight);
  auto [it, res] = point_lights_.try_emplace(id, id, cast_shadows);

  auto &created = it->second;
  created.buffer_index_ = tracker_point_light_;
  ++tracker_point_light_;

  upload_queue_.push_back(&it->second);

  return it->second;
}

void LightSystem::DeletePointLight(PointLight &light) {
  // GPU, overwrite with zeroes
  GLintptr offset = sizeof(gpu::PointLight) * light.buffer_index_;
  glClearNamedBufferSubData(gpu_point_lights_, GL_R32UI, offset,
                            sizeof(gpu::PointLight), GL_RED, GL_UNSIGNED_INT,
                            &global::kZero4Bytes);
  ++point_light_deleted_;
  // CPU
  point_lights_.erase(light.GetId());
}

PointLight &LightSystem::GetPointLight(id::PointLight id) {
  return point_lights_.at(id);
}

void LightSystem::UpdatePointLight(PointLight &light) {
  gpu_point_lights_.UploadIndex(light.ConvertToGpu(), light.buffer_index_);
}

void LightSystem::UploadPointLightsToGpu() noexcept {
  if (upload_queue_.size() == 0) return;

  upload_point_lights_.clear();

  for (const auto *light : upload_queue_) {
    upload_point_lights_.push_back(light->ConvertToGpu());
  }

  gpu_point_lights_.AppendVector(upload_point_lights_);

  upload_queue_.clear();
}

GLuint LightSystem::GetPointLightCount() const { return tracker_point_light_; }

GLuint LightSystem::GetPointLightDeleted() const {
  return point_light_deleted_;
}