#include "camera_system.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "app/input.h"
#include "app/parameters.h"
#include "math/random.h"
#include "options.h"
#include "ui/win_resources.h"

Camera::Camera(id::Camera id, const CameraConfig &config) : id_(id) {
  name_ = (config.name.size()) ? config.name : fmt::format("Camera{}", id);

  position_ = config.position;

  fov_ = config.fov;
  z_near_ = config.z_near;
  z_far_ = config.z_far;

  max_speed_ = config.max_speed;
}

id::Camera Camera::GetId() const { return id_; }

const std::string &Camera::GetName() const { return name_; }

Corners Camera::GetFrustumCornersWS(float near, float far) const {
  using enum FrustumPoints::Enum;
  glm::mat3 dirs = GetDirections();
  glm::vec3 right = dirs[0];
  glm::vec3 up = dirs[1];
  glm::vec3 front = dirs[2];

  glm::vec3 near_c = position_ + front * near;
  glm::vec3 far_c = position_ + front * far;
  float near_h = glm::tan(fov_ * 0.5f) * near;
  float near_w = near_h * app::opengl.aspect_ratio;
  float far_h = glm::tan(fov_ * 0.5f) * far;
  float far_w = far_h * app::opengl.aspect_ratio;

  Corners corners;
  corners[kNearBottomLeft] = near_c - up * near_h - right * near_w;
  corners[kNearTopLeft] = near_c + up * near_h - right * near_w;
  corners[kNearTopRight] = near_c + up * near_h + right * near_w;
  corners[kNearBottomRight] = near_c - up * near_h + right * near_w;
  corners[kFarBottomLeft] = far_c - up * far_h - right * far_w;
  corners[kFarTopLeft] = far_c + up * far_h - right * far_w;
  corners[kFarTopRight] = far_c + up * far_h + right * far_w;
  corners[kFarBottomRight] = far_c - up * far_h + right * far_w;
  return corners;
}

Corners Camera::GetFrustumCornersVS(float near, float far) const {
  using enum FrustumPoints::Enum;
  // center
  float near_c = -near;
  float far_c = -far;

  float near_h = glm::tan(fov_ * 0.5f) * near;
  float near_w = near_h * app::opengl.aspect_ratio;
  float far_h = glm::tan(fov_ * 0.5f) * far;
  float far_w = far_h * app::opengl.aspect_ratio;

  Corners corners;
  corners[kNearBottomLeft] = glm::vec3(-near_w, -near_h, near_c);
  corners[kNearTopLeft] = glm::vec3(near_w, near_h, near_c);
  corners[kNearTopRight] = glm::vec3(near_w, near_h, near_c);
  corners[kNearBottomRight] = glm::vec3(-near_w, -near_h, near_c);
  corners[kFarBottomLeft] = glm::vec3(-far_w, -far_h, far_c);
  corners[kFarTopLeft] = glm::vec3(far_w, far_h, far_c);
  corners[kFarTopRight] = glm::vec3(far_w, far_h, far_c);
  corners[kFarBottomRight] = glm::vec3(-far_w, -far_h, far_c);
  return corners;
}

// reconstruct world space from linear depth
// draw mode is GL_TRIANGLE_STRIP
// [BottomLeft, TopLeft, BottomRight, TopRight]
glm::mat4 Camera::GetFrustumRays(const Corners &corners) const {
  using enum FrustumPoints::Enum;
  glm::vec3 bottom_left = corners[kFarBottomLeft] - corners[kNearBottomLeft];
  glm::vec3 top_left = corners[kFarTopLeft] - corners[kNearTopLeft];
  glm::vec3 bottom_right = corners[kFarBottomRight] - corners[kNearBottomRight];
  glm::vec3 top_right = corners[kFarTopRight] - corners[kNearTopRight];
  return glm::mat4{
      glm::vec4(bottom_left, 1.0f),   //
      glm::vec4(top_left, 1.0f),      //
      glm::vec4(bottom_right, 1.0f),  //
      glm::vec4(top_right, 1.0f)      //
  };
}

glm::mat4 Camera::GetFrustumRaysWS() const {
  Corners cws = GetFrustumCornersWS(z_near_, z_far_);
  return GetFrustumRays(cws);
}

glm::mat4 Camera::GetFrustumRaysVS() const {
  Corners cvs = GetFrustumCornersVS(z_near_, z_far_);
  return GetFrustumRays(cvs);
}

// based on:
// https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
Sphere Camera::GetBoundingSphere(float near, float far) const {
  float height = glm::tan(fov_ * 0.5f) * far;
  float width = height * app::opengl.aspect_ratio;

  float hfov = FovHorizontal();
  float k = glm::sqrt(1.0f + (height * height) / (width * width)) *
            glm::tan(hfov * 0.5f);
  float k2 = k * k;
  float k4 = k2 * k2;
  float fn_sub = far - near;
  float fn_sub2 = fn_sub * fn_sub;
  float fn_add = far + near;
  float fn_add2 = fn_add * fn_add;

  if (k2 >= (fn_sub / fn_add)) {
    glm::vec3 center = Front() * far + position_;
    float radius = far * k;
    return Sphere(center, radius);
  } else {
    float dist = 0.5f * fn_add * (1.0f + k2);
    glm::vec3 center = Front() * dist + position_;
    float radius =
        0.5f * glm::sqrt(fn_sub2 + 2.0f * (far * far + near * near) * k2 +
                         fn_add2 * k4);
    return Sphere(center, radius);
  }
}

MainCamera::MainCamera()
    : event::Base<MainCamera>(&MainCamera::InitEvents, this),
      Camera(0, CameraConfig{}) {}

void MainCamera::InitEvents() {
  Connect(Event::kMoveRight,
          [this]() { wish_vel_ += Right() * axis_speed_.x; });
  Connect(Event::kMoveLeft, [this]() { wish_vel_ -= Right() * axis_speed_.x; });
  Connect(Event::kMoveUp, [this]() { wish_vel_ += Up() * axis_speed_.y; });
  Connect(Event::kMoveDown, [this]() { wish_vel_ -= Up() * axis_speed_.y; });
  Connect(Event::kMoveForward,
          [this]() { wish_vel_ += Front() * axis_speed_.z; });
  Connect(Event::kMoveBackward,
          [this]() { wish_vel_ -= Front() * axis_speed_.z; });

  Connect(Event::kRollLeft, [this]() {
    if (remove_roll_ == false) {
      Roll(roll_speed_rad_ * app::glfw.delta_time);
    }
  });
  Connect(Event::kRollRight, [this]() {
    if (remove_roll_ == false) {
      Roll(-roll_speed_rad_ * app::glfw.delta_time);
    }
  });
  Connect(Event::kModsRun, [this]() { run_mod_ = 2.0f; });
  Connect(Event::kResetCamera, [this]() { Reset(); });

  Connect(Event::kMouseMove, [this]() {
    glm::vec2 offsets = glm::radians(input::GetMouseOffset());
    offsets *= opt::engine.mouse_sensitivity;
    if (fps_style_) {
      MouseMoveEuler(offsets);
    } else {
      MouseMoveQuat(offsets);
    }
  });
  Connect(Event::kScrollZoom, [this]() {
    float offset = glm::radians(input::GetScrollOffset());
    Zoom(offset);
    update_clusters_ = true;
  });
  Connect(Event::kRandomPunch, [this]() {
    event::PushCustomEvent(MainCamera::RandomPunch{*this, 0.1f, 0.25f});
  });
  Connect(Event::kRush, [this]() {
    event::PushCustomEvent(MainCamera::Rush{*this, 0.5f});
  });
}

void MainCamera::Reset() {
  SetQuat(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
  fov_ = global::kDefaultFov;
  Update();
}

void MainCamera::MouseMoveQuat(glm::vec2 offsets) {
  float yaw = offsets.x;
  float pitch = offsets.y;

  if (remove_roll_) {
    // check upside down, swap left/right
    float angle = glm::dot(Up(), global::kWorldUp);
    if (angle < 0.0f) yaw = -yaw;
    YawNoRoll(yaw);
  } else {
    Yaw(yaw);
  }
  Pitch(pitch);
}

void MainCamera::MouseMoveEuler(glm::vec2 offsets) {
  yaw_ += offsets.x;
  pitch_ += offsets.y;

  // constrain pitch
  constexpr float kMaxPitch = glm::radians(89.9f);
  if (pitch_ > kMaxPitch) pitch_ = kMaxPitch;
  if (pitch_ < -kMaxPitch) pitch_ = -kMaxPitch;

  // angleAxis more performance friendly than quat(dir, axis)
  glm::quat q_yaw = glm::angleAxis(yaw_, global::kWorldUp);
  glm::quat q_pitch = glm::angleAxis(pitch_, global::kWorldRight);
  orientation_ = q_pitch * q_yaw;
}

MainCamera::RandomPunch::RandomPunch(MainCamera &cam, float wait_time,
                                     float time)
    : cam(cam), wait_time(wait_time), time(time) {
  glm::vec3 front;
  front.x = rnd::GetFloat(-1.0f, 1.0f);
  front.y = rnd::GetFloat(-1.0f, 1.0f);
  front.z = rnd::GetFloat(-1.0f, 1.0f);
  glm::vec3 speed = rnd::GetFloat(10.0f, 15.0f) * rnd::GetDefaultVec3();
  wish_vel = cam.ExternalVelocity(front, speed);
  acceleration = rnd::GetFloat(10.0f, 15.0f);
}

bool MainCamera::RandomPunch::operator()() {
  if (wait_time > 0.0f) {
    wait_time -= app::glfw.delta_time;
    return false;
  } else {
    if (time > 0.0f) {
      cam.AirAccelerate(wish_vel, acceleration, cam.surface_friction_);
      time -= app::glfw.delta_time;
      return false;
    } else {
      return true;
    }
  }
}

MainCamera::Rush::Rush(MainCamera &cam, float time) : cam(cam), time(time) {
  // fast forward, slightly jump
  glm::vec3 speed;
  speed.x = 0.0f;
  speed.y = rnd::GetFloat(0.0f, 0.15f);
  speed.z = rnd::GetFloat(12.0f, 18.0f);
  wish_vel = cam.SelfVelocity(speed);
  acceleration = rnd::GetFloat(12.0f, 18.0f);
}

bool MainCamera::Rush::operator()() {
  if (time <= 0.0f) return true;

  cam.AirAccelerate(wish_vel, acceleration, cam.surface_friction_);
  time -= app::glfw.delta_time;
  return false;
}

void MainCamera::Update() {
  Friction(velocity_friction_, surface_friction_);

  if (uniform_speed_) {
    UniformAccelerate(wish_vel_, run_mod_ * axis_speed_.x, acceleration_,
                      surface_friction_);
  } else {
    AirAccelerate(run_mod_ * wish_vel_, acceleration_, surface_friction_);
  }

  if (speed_) {
    Move();
    // reset after movement
    wish_vel_ = glm::vec3(0.0f);
    run_mod_ = 1.0f;
  }

  if (head_bobbing_) {
    if (speed_ >= bob_start_speed_) {
      travel_dist_ += speed_ * app::glfw.delta_time;
      // reduce bobbing over time
      bob_fade_mod_ = glm::pow(bob_fade_base_, travel_dist_);
      if (bob_fade_mod_ <= bob_fade_min_) bob_fade_mod_ = bob_fade_min_;

      glm::vec2 magnitude;
      magnitude.x = glm::cos(travel_dist_ + glm::half_pi<float>());
      magnitude.y = glm::sin(glm::half_pi<float>() - 2.0f * travel_dist_);
      magnitude.x *= bob_mod_.x;
      magnitude.y *= bob_mod_.y;

      glm::vec2 bob_add = magnitude - bob_previous_;
      bob_add *= bob_fade_mod_;
      position_.x += bob_add.x;
      position_.y += bob_add.y;

      bob_previous_ = magnitude;
    } else {
      travel_dist_ = 0.0f;
      bob_fade_mod_ = 1.0f;
      bob_previous_ = glm::vec2(0.0f);
    }
  }

  view_ = ViewMat();
  proj_ = ProjMat();
  proj_inverse_ = glm::inverse(proj_);
  proj_view_ = proj_ * view_;

  ray_ = GetRay();
  frustum_ = Frustum(proj_view_);
}

void MainCamera::Use(Camera &camera) {
  Camera &self = *static_cast<Camera *>(this);
  if (current_) {
    *current_ = self;
  }
  current_ = &camera;
  self = camera;
  update_clusters_ = true;
}

CameraSystem::CameraSystem(MainCamera &main_cam, ui::WinResources &ui) noexcept
    : camera_(main_cam), ui_(ui) {
  show_cameras_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                           ShaderStorageBinding::kShowCameras);
  show_cameras_.SetStorage();

  cameras_.reserve(8);
  camera_.Use(CreateCamera(CameraConfig{}));
}

Camera &CameraSystem::CreateCamera(const CameraConfig &config) {
  auto id = id::GenId(id::kCamera);
  auto [it, res] = cameras_.try_emplace(id, id, config);
  auto &camera = it->second;

  ui_.table_camera_.AddRow(id, camera.GetName().c_str());
  return camera;
}

void CameraSystem::DeleteCamera(id::Camera id) {
  cameras_.erase(id);
  ui_.table_camera_.DeleteRow(&ui::CameraRow::id, id);
}

void CameraSystem::SetMainCamera(id::Camera id) {
  auto it = cameras_.find(id);
  if (it != cameras_.end()) {
    camera_.Use(it->second);
  } else {
    spdlog::warn("{}: Camera not found '{}'", __FUNCTION__, id);
  }
}

void CameraSystem::DebugCameras() noexcept {
  cameras_points_.clear();

  for (const auto &[id, camera] : cameras_) {
    if (id == camera_.GetId()) continue;
    // points in world space, already rotated with correct FOV
    Corners corners =
        camera.GetFrustumCornersWS(camera.GetNear(), camera.GetFar());
    for (auto &point : corners) {
      // resize, length == 1.0f
      point = glm::normalize(point);
      // reposition
      point += camera.GetPosition();
      cameras_points_.emplace_back(point, 1.0f);
    }
  }

  if (cameras_points_.size()) {
    show_cameras_.UploadVector(&gpu::StorageShowCameras::cameras_points,
                               cameras_points_);
  }
}

void CameraSystem::DrawCameras() {
  GLsizei cameras_points = static_cast<GLsizei>(cameras_points_.size());
  // each point draw 3 times
  glDrawArrays(GL_LINES, 0, cameras_points * 3);
}