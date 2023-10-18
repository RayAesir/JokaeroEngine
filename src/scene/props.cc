#include "props.h"

// local
#include "app/parameters.h"

Props::Props(GLuint props) : props_(props) {}

GLuint Props::GetProps() const { return props_; }

void Props::SetProps(Bits bit, bool value) {
  props_ = (props_ & ~(1U << bit)) | (value << bit);
}

bool Props::IsEnabled() const { return props_ & Flags::kEnabled; }

bool Props::IsVisible() const { return props_ & Flags::kVisible; }

bool Props::IsCastShadows() const { return props_ & Flags::kCastShadows; }

void Movable::SetVelocity(const glm::vec3& velocity) { velocity_ = velocity_; }

void Movable::SetMaxSpeed(float max_speed) { max_speed_ = max_speed; }

float Movable::GetCurrentSpeed() const { return speed_; }

bool Movable::IsMoving() const { return (speed_ > 0.0f); }

bool Movable::IsStopped() const { return (speed_ == 0.0f); }

glm::vec3 Movable::ExternalVelocity(const glm::vec3& front,
                                    const glm::vec3& speed) {
  glm::vec3 right = glm::normalize(glm::cross(front, global::kWorldUp));
  glm::vec3 up = glm::normalize(glm::cross(right, front));
  glm::vec3 wish_vel{0.0f};
  wish_vel += right * speed.x;
  wish_vel += up * speed.y;
  wish_vel += front * speed.z;
  return wish_vel;
}

glm::vec3 Movable::SelfVelocity(const glm::vec3& speed) {
  glm::vec3 wish_vel{0.0f};
  wish_vel += Right() * speed.x;
  wish_vel += Up() * speed.y;
  wish_vel += Front() * speed.z;
  return wish_vel;
}

// based on: https://github.com/ValveSoftware/source-sdk-2013
void Movable::UniformAccelerate(const glm::vec3& dir, float speed,
                                float acceleration, float surface_friction) {
  // direction can't be zero -> NaN
  const float len2 = glm::dot(dir, dir);
  constexpr float eps2 = glm::epsilon<float>() * glm::epsilon<float>();
  if (len2 <= eps2) return;
  // manual normalize after check (optimization)
  glm::vec3 wish_dir = dir * glm::inversesqrt(len2);
  // see if we are changing direction a bit
  float current_speed = glm::dot(velocity_, wish_dir);
  // clamp max speed
  if (speed > max_speed_) speed = max_speed_;
  // reduce wish speed by the amount of veer
  float add_speed = speed - current_speed;
  // if not going to add any speed, done
  if (add_speed <= 0.0f) return;
  // determine amount of acceleration
  float accel_speed = speed * acceleration * app::glfw.delta_time;
  // apply non inverted friction to linear control over speed
  accel_speed *= surface_friction;
  // cap speed
  if (accel_speed > add_speed) accel_speed = add_speed;
  // adjust velocity
  velocity_ += wish_dir * accel_speed;
}

void Movable::AirAccelerate(const glm::vec3& wish_vel, float acceleration,
                            float surface_friction) {
  // direction can't be zero -> NaN
  const float len2 = glm::dot(wish_vel, wish_vel);
  constexpr float eps2 = glm::epsilon<float>() * glm::epsilon<float>();
  if (len2 <= eps2) return;
  // manual normalize/length after check (optimization)
  glm::vec3 wish_dir = wish_vel * glm::inversesqrt(len2);
  float wish_speed = glm::sqrt(len2);
  // see if we are changing direction a bit
  float current_speed = glm::dot(velocity_, wish_dir);
  // clamp max speed
  if (wish_speed > max_speed_) wish_speed = max_speed_;
  // reduce wish speed by the amount of veer
  float add_speed = wish_speed - current_speed;
  // if not going to add any speed, done
  if (add_speed <= 0.0f) return;
  // determine amount of acceleration
  float accel_speed = wish_speed * acceleration * app::glfw.delta_time;
  // apply non inverted friction to linear control over speed
  accel_speed *= surface_friction;
  // cap speed
  if (accel_speed > add_speed) accel_speed = add_speed;
  // adjust velocity
  velocity_ += wish_dir * accel_speed;
}

void Movable::Friction(float velocity_friction, float surface_friction) {
  // zero speed -> NaN
  float len2 = glm::dot(velocity_, velocity_);
  if (len2 <= glm::epsilon<float>()) {
    velocity_ = glm::vec3(0.0f);
    speed_ = 0.0f;
    return;
  }
  // manual length after check (optimization)
  speed_ = glm::sqrt(len2);
  // bleed off some speed
  float control = (speed_ < 0.5f) ? 0.5f : speed_;
  // drop
  float friction = velocity_friction * surface_friction;
  float drop = control * friction * app::glfw.delta_time;
  // scale mod for velocity
  float new_speed = speed_ - drop;
  if (new_speed < 0.0f) new_speed = 0.0f;
  // determine proportion of old speed we using
  new_speed /= speed_;
  // adjust velocity according to proportion
  velocity_ *= new_speed;
}

void Movable::Move() { position_ += velocity_ * app::glfw.delta_time; }

float Viewable::GetNear() const { return z_near_; }

float Viewable::GetFar() const { return z_far_; }

void Viewable::SetNear(float dist) { z_near_ = dist; }

void Viewable::SetFar(float dist) { z_far_ = dist; }

float Viewable::DepthRange() const { return z_far_ - z_near_; }

float Viewable::DepthRatio() const { return z_far_ / z_near_; }

float Viewable::GetFov() const { return fov_; }

void Viewable::SetFov(float fov_rad) { fov_ = fov_rad; }

float Viewable::FovDeg() const { return glm::degrees(fov_); }

float Viewable::FovHorizontal() const {
  return 2.0f * glm::atan(glm::tan(fov_ * 0.5f) * app::opengl.aspect_ratio);
}

float Viewable::FovHorizontalDeg() const {
  return glm::degrees(FovHorizontal());
}

void Viewable::Zoom(float offset) {
  fov_ -= offset;
  if (fov_ < global::kMinFov) fov_ = global::kMinFov;
  if (fov_ > global::kMaxFov) fov_ = global::kMaxFov;
}

glm::mat4 Viewable::ViewMat() const {
  return glm::lookAt(position_, position_ + Front(), Up());
}

glm::mat4 Viewable::ProjMat() const {
  // Reversed-Z (far, near)
  return glm::perspective(fov_,                      //
                          app::opengl.aspect_ratio,  //
                          z_far_,                    //
                          z_near_                    //
  );
}

Ray Viewable::GetRay() const { return Ray(position_, Front()); }

Frustum Viewable::GetFrustum() const { return Frustum{ProjMat() * ViewMat()}; }