#pragma once

// deps
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// valuable information:
// https://gamedev.stackexchange.com/questions/136174/im-rotating-an-object-on-two-axes-so-why-does-it-keep-twisting-around-the-thir
class Transformation {
 public:
  Transformation() = default;
  Transformation(const glm::vec3& translation, const glm::mat3& mat,
                 const glm::vec3& scale);
  Transformation(const glm::vec3& translation, const glm::vec3& euler_angles,
                 const glm::vec3& scale);
  Transformation(const glm::vec3& translation, const glm::quat& quat,
                 const glm::vec3& scale);
  virtual ~Transformation() = default;

 public:
  const glm::vec3& GetPosition() const;
  void SetPosition(const glm::vec3& translation);
  void Translate(const glm::vec3& translation);

  // all calculations in radians
  // euler angles in XYZ order
  const glm::quat& GetQuat() const;
  glm::mat3 GetRotation() const;
  glm::vec3 GetEuler() const;
  glm::vec3 GetEulerDeg() const;
  void SetQuat(const glm::quat& quat);
  void SetRotation(const glm::mat3& mat);
  void SetEuler(const glm::vec3& euler_rad);
  void SetEulerDeg(const glm::vec3& euler_deg);

  glm::mat3 GetDirections() const;
  glm::vec3 Right() const;
  glm::vec3 Up() const;
  glm::vec3 Front() const;
  // separated to apply constraints
  // use for 2D rotation with locked roll (Camera)
  void Pitch(float angle);
  void YawNoRoll(float angle);
  // use free 3D with roll (Fly)
  void Yaw(float angle);
  void Roll(float angle);

  const glm::vec3& GetScale() const;
  void SetScale(const glm::vec3& scale);
  void SetUniScale(float scale);
  void Scale(const glm::vec3& scale);
  void Scale(float uni_scale);

  glm::mat4 GetTransformation() const;

 protected:
  glm::vec3 position_{0.0f};
  // use quaternion to avoid the gimbal lock and euler ambiguity
  glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 scale_{1.0f};
};
