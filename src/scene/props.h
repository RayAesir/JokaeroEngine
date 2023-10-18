#pragma once

// local
#include "math/collision_types.h"
#include "math/transformation.h"

class Props {
 public:
  // set, expose
  enum Bits : GLuint {
    // when object/light is deleted,
    // instances in GPU buffer are overwritten with zeroes
    // so culling stage skip deleted automatically
    kEnabled = 0,
    // CPU flag controlled via GPU readback visibility buffer
    kVisible = 1,
    // read by GPU for point lights
    kCastShadows = 2,
  };

  // constructor
  struct Flags {
    enum Enum : GLuint {
      kEnabled = (1U << Bits::kEnabled),
      kVisible = (1U << Bits::kVisible),
      kCastShadows = (1U << Bits::kCastShadows)
    };
  };

  Props(GLuint props);

 public:
  GLuint GetProps() const;
  void SetProps(Bits bit, bool value);

  bool IsEnabled() const;
  bool IsVisible() const;
  bool IsCastShadows() const;

 private:
  GLuint props_{0};
};

// abstract Movable can be used for Camera or Objects
class Movable : public virtual Transformation {
 public:
  Movable() = default;

 public:
  void SetVelocity(const glm::vec3& velocity);
  void SetMaxSpeed(float max_speed);
  float GetCurrentSpeed() const;
  bool IsMoving() const;
  bool IsStopped() const;

  // helper methods
  glm::vec3 ExternalVelocity(const glm::vec3& front, const glm::vec3& speed);
  glm::vec3 SelfVelocity(const glm::vec3& speed);
  // can be multiple sources of acceleration
  // preserve uniform speed in all directions
  void UniformAccelerate(const glm::vec3& dir, float speed, float acceleration,
                         float surface_friction);
  // diagonal movement faster, but physically correst
  void AirAccelerate(const glm::vec3& wish_vel, float acceleration,
                     float surface_friction);
  void Friction(float velocity_friction, float surface_friction);
  // apply velocity, change position
  void Move();

 protected:
  glm::vec3 velocity_{0.0f};
  float speed_{0.0f};
  float max_speed_{20.0f};
};

class Viewable : public virtual Transformation {
 public:
  Viewable() = default;

 public:
  float GetNear() const;
  float GetFar() const;
  void SetNear(float dist);
  void SetFar(float dist);
  float DepthRange() const;
  float DepthRatio() const;
  // negative - zoom out, positive - zoom in
  float GetFov() const;
  void SetFov(float fov_rad);

  float FovDeg() const;
  float FovHorizontal() const;
  float FovHorizontalDeg() const;
  void Zoom(float offset);

  glm::mat4 ViewMat() const;
  glm::mat4 ProjMat() const;
  Ray GetRay() const;
  Frustum GetFrustum() const;

 protected:
  // all calculations in radians
  float fov_{global::kDefaultFov};
  float z_near_{0.1f};
  float z_far_{400.0f};
};