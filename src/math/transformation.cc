#include "transformation.h"

// local
#include "global.h"

const glm::vec3& Transformation::GetPosition() const { return position_; }

void Transformation::SetPosition(const glm::vec3& translation) {
  position_ = translation;
}

void Transformation::Translate(const glm::vec3& translation) {
  position_ += translation;
}

const glm::quat& Transformation::GetQuat() const { return orientation_; }

glm::mat3 Transformation::GetRotation() const {
  return glm::mat3_cast(orientation_);
}

glm::vec3 Transformation::GetEuler() const {
  return glm::eulerAngles(orientation_);
}

glm::vec3 Transformation::GetEulerDeg() const {
  return glm::degrees(glm::eulerAngles(orientation_));
}

void Transformation::SetQuat(const glm::quat& quat) { orientation_ = quat; }

void Transformation::SetRotation(const glm::mat3& mat) {
  orientation_ = glm::quat(mat);
}

void Transformation::SetEuler(const glm::vec3& euler_rad) {
  orientation_ = glm::quat(euler_rad);
}

void Transformation::SetEulerDeg(const glm::vec3& euler_deg) {
  orientation_ = glm::quat(glm::radians(euler_deg));
}

glm::mat3 Transformation::GetDirections() const {
  // need rows, transpose for easy access
  glm::mat3 dirs = glm::transpose(glm::mat3_cast(orientation_));
  // OpenGL RH, negative View-Z
  dirs[2] = -dirs[2];
  return dirs;
}

glm::vec3 Transformation::Right() const {
  return glm::normalize(glm::conjugate(orientation_) * global::kWorldRight);
}

glm::vec3 Transformation::Up() const {
  return glm::normalize(glm::conjugate(orientation_) * global::kWorldUp);
}

glm::vec3 Transformation::Front() const {
  return glm::normalize(glm::conjugate(orientation_) * global::kWorldFront);
}

void Transformation::Pitch(float angle) {
  glm::quat q_pitch = glm::angleAxis(angle, Right());
  orientation_ = orientation_ * q_pitch;
}

void Transformation::YawNoRoll(float angle) {
  glm::quat q_yaw = glm::angleAxis(angle, global::kWorldUp);
  orientation_ = orientation_ * q_yaw;
}

void Transformation::Yaw(float angle) {
  glm::quat q_yaw = glm::angleAxis(angle, Up());
  orientation_ = orientation_ * q_yaw;
}

void Transformation::Roll(float angle) {
  glm::quat q_roll = glm::angleAxis(angle, Front());
  orientation_ = orientation_ * q_roll;
}

const glm::vec3& Transformation::GetScale() const { return scale_; }

void Transformation::SetScale(const glm::vec3& scale) { scale_ = scale; }

void Transformation::SetUniScale(float scale) {
  scale_.x = scale;
  scale_.y = scale;
  scale_.z = scale;
}

void Transformation::Scale(const glm::vec3& scale) { scale_ *= scale; }

void Transformation::Scale(float uni_scale) { scale_ *= uni_scale; }

Transformation::Transformation(const glm::vec3& translation,
                               const glm::mat3& mat, const glm::vec3& scale)
    : position_(translation), orientation_(mat), scale_(scale) {}

Transformation::Transformation(const glm::vec3& translation,
                               const glm::vec3& euler_angles,
                               const glm::vec3& scale)
    : position_(translation), orientation_(euler_angles), scale_(scale) {}

Transformation::Transformation(const glm::vec3& translation,
                               const glm::quat& quat, const glm::vec3& scale)
    : position_(translation), orientation_(quat), scale_(scale) {}

glm::mat4 Transformation::GetTransformation() const {
  glm::mat4 t = glm::mat4_cast(orientation_);

  t[0] *= scale_.x;
  t[1] *= scale_.y;
  t[2] *= scale_.z;

  t[3].x = position_.x;
  t[3].y = position_.y;
  t[3].z = position_.z;
  t[3].w = 1.0f;
  return t;
}
