#pragma once

// global
#include <glm/gtc/quaternion.hpp>
#include <string>
// local
#include "events.h"
#include "global.h"
#include "id_generator.h"
#include "opengl/buffer_storage.h"
#include "scene/props.h"
// fwd
namespace ui {
class WinResources;
class WinScene;
}  // namespace ui

namespace gpu {

struct StorageShowCameras {
  glm::vec4 cameras_points[global::kDebugCamerasPoints];
};

}  // namespace gpu

struct CameraConfig {
  std::string name{""};
  glm::vec3 position{0.0f, 2.0f, 0.0f};
  float fov{global::kDefaultFov};
  float z_near{0.1f};
  float z_far{400.0f};
  float max_speed{50.0f};
};

class Camera : public Movable, public Viewable {
 public:
  Camera(id::Camera id, const CameraConfig &config);

 public:
  id::Camera GetId() const;
  const std::string &GetName() const;
  // near and far as args, for Shadow Cascades
  Corners GetFrustumCornersWS(float near, float far) const;
  Corners GetFrustumCornersVS(float near, float far) const;
  glm::mat4 GetFrustumRays(const Corners &corners) const;
  glm::mat4 GetFrustumRaysWS() const;
  glm::mat4 GetFrustumRaysVS() const;
  Sphere GetBoundingSphere(float near, float far) const;

 protected:
  id::Camera id_;
  std::string name_;
};

// Task: have multiple Cameras with different settings
// inheritance:
// ++ comfy access to all members/methods
// ++ SOLID, MainCamera subclass of Camera with render capabilities
// ++ safety, if all cameras somehow deleted, program just works
// -- copy data from/to CameraSystem
class MainCamera : public event::Base<MainCamera>, public Camera {
 public:
  friend ui::WinScene;
  MainCamera();
  using Inject = MainCamera();

 public:
  // recalculate clusters when change:
  // camera, zoom, near, far, UI
  bool update_clusters_{true};

  bool fps_style_{true};
  bool remove_roll_{true};
  bool uniform_speed_{true};
  bool head_bobbing_{true};

  // FPS style
  float yaw_{0.0f};
  float pitch_{0.0f};

  glm::vec3 axis_speed_{5.0f};
  float roll_speed_rad_{glm::radians(60.0f)};
  // acceleration and velocity_friction should be correlated
  float acceleration_{4.0f};
  float velocity_friction_{4.0f};
  // bigger value, more friction and acceleration
  float surface_friction_{1.0f};
  // reset each frame
  glm::vec3 wish_vel_{0.0f};
  float run_mod_{1.0f};

  glm::mat4 view_{1.0f};
  glm::mat4 proj_{1.0f};
  glm::mat4 proj_inverse_{1.0f};
  glm::mat4 proj_view_{1.0f};

  Ray ray_;
  Frustum frustum_;

  // head bobbing
  float travel_dist_{0.0f};
  float bob_start_speed_{0.5f};
  float bob_fade_mod_{1.0f};
  float bob_fade_base_{0.8f};
  float bob_fade_min_{0.2f};
  glm::vec2 bob_mod_{0.2f, 0.15f};
  glm::vec2 bob_previous_{0.0f};

  void Update();
  // copy data from Camera and return back when changed
  void Use(Camera &camera);

 private:
  Camera *current_{nullptr};

  void InitEvents();
  void Reset();
  // functors as custom (timed) events
  // operator return true when event expired and should be removed
  struct RandomPunch {
    RandomPunch(MainCamera &cam, float wait_time, float time);
    bool operator()();

    MainCamera &cam;
    glm::vec3 wish_vel;
    float acceleration;
    float wait_time;
    float time;
  };
  struct Rush {
    Rush(MainCamera &cam, float time);
    bool operator()();

    MainCamera &cam;
    glm::vec3 wish_vel;
    float acceleration;
    float time;
  };
  void MouseMoveQuat(glm::vec2 offsets);
  void MouseMoveEuler(glm::vec2 offsets);
};

class CameraSystem {
 public:
  CameraSystem(MainCamera &main_cam, ui::WinResources &ui) noexcept;
  using Inject = CameraSystem(MainCamera &, ui::WinResources &);

 public:
  MainCamera &camera_;

  Camera &CreateCamera(const CameraConfig &config);
  void DeleteCamera(id::Camera id);
  void SetMainCamera(id::Camera id);

  void DebugCameras() noexcept;
  void DrawCameras();

 private:
  ui::WinResources &ui_;

  MiUnMap<id::Camera, Camera> cameras_;

  gl::StreamBuffer<gpu::StorageShowCameras> show_cameras_{"ShowCameras"};
  MiVector<glm::vec4> cameras_points_;
};