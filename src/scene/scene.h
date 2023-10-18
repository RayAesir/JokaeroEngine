#pragma once

// local
#include "assets/texture.h"
#include "events.h"
#include "opengl/buffer_storage.h"
#include "scene/camera_system.h"
#include "scene/light_system.h"
#include "scene/object_system.h"
#include "scene/particle_system.h"
// fwd
class Assets;

namespace gpu {

struct StoragePickEntity {
  GLuint point_light_buffer_id;
  GLuint point_light_id;
  //
  GLuint opaque_instance_id;
  GLuint opaque_object_id;
  GLfloat opaque_dist;
  //
  GLuint transparent_instance_id;
  GLuint transparent_object_id;
  GLfloat transparent_dist;
};

}  // namespace gpu

// Scene is owner of Systems
// System works as Component+System (parody on ECS)
// Store Components (Object, Lights, etc.)
// Provide methods to Create/Update/Delete
class Scene : public event::Base<Scene> {
 public:
  Scene(CameraSystem &cameras,      //
        ObjectSystem &objects,      //
        LightSystem &lights,        //
        ParticleSystem &particles,  //
        MainCamera &camera,         //
        Assets &assets              //
  );
  using Inject = Scene(CameraSystem &, ObjectSystem &, LightSystem &,
                       ParticleSystem &, MainCamera &, Assets &);

  struct Lighting {
    glm::vec3 direction{1.0f, -1.0f, 0.0f};
    glm::vec4 diffuse{0.9f, 0.9f, 0.9f, 2.0f};
    glm::vec4 ambient{0.9f, 0.9f, 0.9f, 1.5f};
    glm::vec4 skybox{1.0f, 1.0f, 1.0f, 1.5f};
  };

 public:
  CameraSystem &cameras_;
  ObjectSystem &objects_;
  LightSystem &lights_;
  ParticleSystem &particles_;

  MainCamera &camera_;

  std::shared_ptr<EnvTexture> env_tex_{nullptr};
  void SetEnvTexture(const std::string &name);

  Lighting lighting_;

  gl::MappedBuffer<gpu::StoragePickEntity> pick_entity_{"PickEntity"};

  float dist_to_object_{0.0f};

  Object *look_at_object_{nullptr};
  PointLight *look_at_point_light_{nullptr};
  FxInstance *look_at_fx_instance_{nullptr};

  Object *selected_object_{nullptr};
  PointLight *selected_point_light_{nullptr};
  FxInstance *selected_fx_instance_{nullptr};

  GLuint object_count_;
  // GPU count (with deleted)
  GLuint instance_count_;
  GLuint point_light_count_;

  void CreateObjectAtCamera(const std::string &model_name);
  void CreatePointLightAtCamera();

  void DeleteSelectedObject();
  void DeleteSelectedPointLight();
  void DeleteSelectedParticles();

  void PlaceStraight(const std::string &name, int count, float area,
                     float scale);
  void PlaceInAir(const std::string &name, int count, float area, float scale,
                  float height);

  void ProcessScene() noexcept;
  void PickEntity();

 private:
  Assets &assets_;

  void SetLookAtObject(Object *object, float dist) noexcept;
  // selection
  void LookAtNothing();
  void LookAtOpaque();
  void LookAtTransparent();
  void LookAtBoth();

  void InitEvents();
};
