#include "scene.h"

// deps
#include <spdlog/spdlog.h>
// global
#include <glm/gtx/component_wise.hpp>
#include <numeric>
// local
#include "assets/assets.h"
#include "math/random.h"

Scene::Scene(CameraSystem &cameras,      //
             ObjectSystem &objects,      //
             LightSystem &lights,        //
             ParticleSystem &particles,  //
             MainCamera &camera,         //
             Assets &assets              //
             )
    : event::Base<Scene>(&Scene::InitEvents, this),
      cameras_(cameras),
      objects_(objects),
      lights_(lights),
      particles_(particles),
      camera_(camera),
      assets_(assets) {
  pick_entity_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                          ShaderStorageBinding::kPickEntity);
  pick_entity_.SetStorage(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                          GL_MAP_COHERENT_BIT);
  pick_entity_.MapRange(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                        GL_MAP_COHERENT_BIT);
}

void Scene::InitEvents() {
  Connect(Event::kSelectEntity, [this]() {
    if (look_at_object_ && (look_at_object_ != selected_object_)) {
      selected_object_ = look_at_object_;
    } else {
      selected_object_ = nullptr;
    }

    if (look_at_point_light_ &&
        (look_at_point_light_ != selected_point_light_)) {
      selected_point_light_ = look_at_point_light_;
    } else {
      selected_point_light_ = nullptr;
    }

    if (look_at_fx_instance_ &&
        (look_at_fx_instance_ != selected_fx_instance_)) {
      selected_fx_instance_ = look_at_fx_instance_;
    } else {
      selected_fx_instance_ = nullptr;
    }
  });
}

void Scene::SetEnvTexture(const std::string &name) {
  env_tex_ = assets_.env_tex_.GetEnvTexture(name);
}

void Scene::CreateObjectAtCamera(const std::string &model_name) {
  Object *obj = objects_.CreateObject(model_name);
  if (obj) {
    // find the max distance to place object in front of camera
    const auto &model = obj->GetModel();
    glm::vec3 extent = model.GetBB().extent_;
    // already calculated, transpose view -> world
    glm::mat3 rot_mat = glm::transpose(glm::mat3(camera_.view_));
    extent = rot_mat * extent;
    float max_dist = glm::compMax(glm::abs(extent));
    glm::vec3 cam_pos = camera_.GetPosition() + camera_.Front();
    glm::vec3 offset = camera_.Front() * max_dist;

    obj->SetPosition(cam_pos + offset);
    obj->SetRotation(rot_mat);
  }
}

void Scene::CreatePointLightAtCamera() {
  auto &light = lights_.CreatePointLight(false);
  light.position_ = camera_.GetPosition() + (camera_.Front() * 5.0f);
  light.radius_ = 5.0f;
}

void Scene::DeleteSelectedObject() {
  if (selected_object_) {
    objects_.DeleteObject(*selected_object_);
    selected_object_ = nullptr;
  }
}

void Scene::DeleteSelectedPointLight() {
  if (selected_point_light_) {
    lights_.DeletePointLight(*selected_point_light_);
    selected_point_light_ = nullptr;
  }
}

void Scene::DeleteSelectedParticles() {
  if (selected_fx_instance_) {
    particles_.DeleteParticles(look_at_fx_instance_);
    selected_fx_instance_ = nullptr;
  }
}

void Scene::PlaceStraight(const std::string &name, int count, float area,
                          float scale) {
  for (int i = 0; i < count; ++i) {
    auto obj = objects_.CreateObject(name);
    if (obj) {
      obj->SetPosition(rnd::Vec3(-area, area, 0.0f, 0.0f, -area, area));
      obj->SetEuler(glm::vec3(rnd::AngleRad(0.0f, 2.0f),
                              rnd::AngleRad(0.0f, 180.0f),
                              rnd::AngleRad(0.0f, 2.0f)));
      obj->SetUniScale(scale);
    }
  }
}

void Scene::PlaceInAir(const std::string &name, int count, float area,
                       float scale, float height) {
  for (int i = 0; i < count; ++i) {
    auto obj = objects_.CreateObject(name);
    if (obj) {
      obj->SetPosition(rnd::Vec3(-area, area, 0.0f, height, -area, area));
      obj->SetEuler(glm::vec3(rnd::AngleRad(0.0f, 45.0f),
                              rnd::AngleRad(0.0f, 180.0f),
                              rnd::AngleRad(0.0f, 45.0f)));
      obj->SetUniScale(scale);
    }
  }
}

void Scene::ProcessScene() noexcept {
  // batch upload when loaded/created
  assets_.models_.UploadCmdBoxMaterial();
  objects_.UploadObjectsToGpu();
  lights_.UploadPointLightsToGpu();

  camera_.Update();
  objects_.ProcessAnimations();
  look_at_fx_instance_ = particles_.ProcessParticles();

  object_count_ = objects_.GetObjectCount();
  instance_count_ = objects_.GetInstanceCount();
  point_light_count_ = lights_.GetPointLightCount();
}

void Scene::SetLookAtObject(Object *object, float dist) noexcept {
  look_at_object_ = object;
  dist_to_object_ = dist;
}

void Scene::LookAtNothing() { SetLookAtObject(nullptr, 0.0f); }

void Scene::LookAtOpaque() {
  auto &pick = *pick_entity_;
  GLuint obj_id = pick.opaque_object_id;
  GLfloat dist = pick.opaque_dist;
  SetLookAtObject(&objects_.GetObject(obj_id), dist);
}

void Scene::LookAtTransparent() {
  auto &pick = *pick_entity_;
  GLuint obj_id = pick.transparent_object_id;
  GLfloat dist = pick.transparent_dist;
  SetLookAtObject(&objects_.GetObject(obj_id), dist);
}

void Scene::LookAtBoth() {
  auto &pick = *pick_entity_;
  if (pick.opaque_dist > pick.transparent_dist) {
    GLuint obj_id = pick.transparent_object_id;
    GLfloat closer_dist = pick.transparent_dist;
    SetLookAtObject(&objects_.GetObject(obj_id), closer_dist);
  } else {
    // opaque should overlap
    GLuint obj_id = pick.opaque_object_id;
    GLfloat closer_dist = pick.opaque_dist;
    SetLookAtObject(&objects_.GetObject(obj_id), closer_dist);
  }
}

void Scene::PickEntity() {
  auto &pick = *pick_entity_;

  if (pick.point_light_buffer_id > 0) {
    look_at_point_light_ = &lights_.GetPointLight(pick.point_light_id);
  } else {
    look_at_point_light_ = nullptr;
  }

  // zero == null instance (none)
  bool opaque = pick.opaque_instance_id;
  bool transparent = pick.transparent_instance_id;

  // say no to conditions
  using MemFn = void (Scene::*)();
  static constexpr MemFn selector[2][2]{
      &Scene::LookAtNothing,      // false, false
      &Scene::LookAtTransparent,  // false, true
      &Scene::LookAtOpaque,       // true, false
      &Scene::LookAtBoth,         // true, true
  };
  (this->*selector[opaque][transparent])();
}
