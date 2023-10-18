#pragma once

// local
#include "global.h"
#include "id_generator.h"
#include "math/collision_types.h"
#include "opengl/buffer_storage.h"
#include "scene/props.h"
// fwd
namespace ui {
class WinScene;
}

namespace gpu {

struct PointLight {
  GLuint id;
  GLuint empty_pad0;
  GLuint empty_pad1;
  GLuint empty_pad2;
  //
  GLuint props;
  GLfloat radius;
  GLfloat radius2;
  GLfloat inv_radius2;
  //
  glm::vec4 diffuse;
  glm::vec4 position;
};

struct StoragePointLights {
  PointLight point_lights[global::kMaxPointLights];
};

}  // namespace gpu

class PointLight : public Props {
 public:
  PointLight(id::PointLight id, bool cast_shadows);
  // GPU resource (move-only)
  PointLight(const PointLight &) = delete;
  PointLight &operator=(const PointLight &) = delete;
  PointLight(PointLight &&) = default;
  PointLight &operator=(PointLight &&) = default;

 public:
  glm::vec3 position_{0.0f};
  float radius_{1.0f};
  glm::vec3 diffuse_{1.0f};
  float intensity_{1.0f};

  GLuint buffer_index_;

  id::PointLight GetId() const;
  Sphere GetSphere() const noexcept;

  gpu::PointLight ConvertToGpu() const noexcept;

 private:
  id::PointLight id_;
};

class LightSystem {
 public:
  LightSystem() noexcept;
  using Inject = LightSystem();

 public:
  PointLight &CreatePointLight(bool cast_shadows);
  void DeletePointLight(PointLight &light);
  PointLight &GetPointLight(id::PointLight id);
  void UpdatePointLight(PointLight &light);

  void UploadPointLightsToGpu() noexcept;

  // GPU count (with deleted)
  GLuint GetPointLightCount() const;
  GLuint GetPointLightDeleted() const;

 private:
  gl::CountedBuffer<gpu::StoragePointLights> gpu_point_lights_{"PointLights"};

  // first instance reserved for none (like nullptr)
  GLuint tracker_point_light_{1};
  GLuint point_light_deleted_{0};

  MiUnMap<id::PointLight, PointLight> point_lights_;
  // Setup stage changes data, delay upload
  MiVector<PointLight *> upload_queue_;

  // allocated space
  MiVector<gpu::PointLight> upload_point_lights_;
};