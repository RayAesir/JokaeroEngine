#pragma once

// global
#include <array>
// local
#include "global.h"
#include "mi_types.h"
// fwd
class Transformation;

namespace gpu {

struct AABB {
  AABB() = default;
  AABB(const glm::vec3 &c, const glm::vec3 &e)
      : center(c, 1.0f), extent(e, 0.0f) {}
  glm::vec4 center;
  glm::vec4 extent;
};

}  // namespace gpu

// usage: GetMaxLimits<glm::vec3, float>()
template <typename T, typename P>
constexpr T GetMaxLimits() {
  return T(std::numeric_limits<P>::max());
}

template <typename T, typename P>
constexpr T GetMinLimits() {
  return T(std::numeric_limits<P>::lowest());
}

struct FrustumPlanes {
  enum Enum : unsigned int {
    kLeft,
    kRight,
    kTop,
    kBottom,
    kNear,
    kFar,
    kTotal
  };
};

struct FrustumPoints {
  enum Enum : unsigned int {
    kFarBottomLeft,
    kFarTopLeft,
    kFarTopRight,
    kFarBottomRight,
    kNearBottomLeft,
    kNearTopLeft,
    kNearTopRight,
    kNearBottomRight,
    kTotal
  };
};

using Corners = std::array<glm::vec3, FrustumPoints::kTotal>;

// R(t) = O + t*D
// t - translation, D - direction, O - origin
class Ray {
 public:
  Ray() = default;
  Ray(const glm::vec3 &origin, const glm::vec3 &dir);

 public:
  glm::vec3 origin_;
  glm::vec3 dir_;
  // optimization
  glm::vec3 dirfrac_;
};

// A*x + B*y + C*z + D = 0
// (A, B, C) is the normal vector
// D represents regular Euclidean distance from the origin to the plane
// only if vector is normalized
class Plane {
 public:
  Plane() = default;
  Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2);
  Plane(const glm::vec3 &p0, const glm::vec3 &normal);
  Plane(const float &dist, const glm::vec3 &normal);

 public:
  glm::vec3 normal_;
  // shortest signed "distance" from origin to plane
  float dist_;
};

// Axis-aligned bounding box
class AABB {
 public:
  AABB() = default;
  AABB(const glm::vec3 &min, const glm::vec3 &max);
  AABB(const MiVector<glm::vec3> &vertices);
  AABB(const Corners &corners);

 public:
  glm::vec3 Min() const;
  glm::vec3 Max() const;
  void Transform(const AABB &bb, const glm::mat4 &mat);
  AABB Transform(const glm::mat4 &mat) const;
  void Move(const glm::vec3 &pos);
  void MoveAndResize(const glm::vec3 &pos, const glm::vec3 &size);
  Corners GetCorners() const;
  void ExpandToInclude(const glm::vec3 &point);
  void ExpandToInclude(const AABB &aabb);
  float SurfaceArea() const;
  unsigned int MaxDimension() const;

 public:
  glm::vec3 center_{0.0f};
  glm::vec3 extent_{0.0f};
};

// Oriented bounding box
class OBB {
 public:
  OBB() = default;
  OBB(const AABB &aabb, const Transformation &t);

 public:
  void Transform(const AABB &aabb, const Transformation &t);

 public:
  glm::vec3 center_{0.0f};
  glm::vec3 extent_{0.0f};
  glm::mat3 orientation_{1.0f};
};

class Sphere {
 public:
  Sphere() = default;
  Sphere(const glm::vec3 &center, float radius);
  Sphere(const AABB &aabb);

 public:
  Corners GetCorners() const;

 public:
  glm::vec3 center_{0.0f};
  float radius_{0.0f};
  // to avoid an expensive square root operation
  // the squared distances are compared
  float radius_sqr_{0.0f};
};

class Frustum {
 public:
  Frustum() = default;
  Frustum(const glm::mat4 &mat);

 public:
  std::array<Plane, FrustumPlanes::kTotal> planes_;
};

// based on:
// http://lspiroengine.com/?p=153
// http://lspiroengine.com/?p=187
// Discrete Oriented Polytope e.g. AABB is a 6 planes DOP
class DOP {
 public:
  DOP() = default;
  DOP(const Frustum &frustum, const Corners &corners,
      const glm::vec3 &light_dir);

 public:
  // the 5 planes of frustum look toward the light
  // and 6 planes are creating outline, the total count is 11
  // Near plane is never added (cast shadows from 10 miles away)
  // Plane(glm::dot(camera.position_, light_dir) - 1.0f,
  // glm::normalize(light_dir))
  std::array<Plane, global::kDopPlanes> planes_;
  unsigned int total_{0};

 private:
  glm::uvec4 GetNeighbors(unsigned int plane) const;
  glm::uvec2 GetCornersOfPlanes(unsigned int plane1, unsigned int plane2) const;
};
