#include "intersection.h"

// deps
#include <glm/gtc/matrix_access.hpp>
// local
#include "math/fast_math.h"

namespace math {

float GetSignedDistance(const Plane &plane, const glm::vec3 &point) {
  // the distance from the point to this plane
  return glm::dot(plane.normal_, point) - plane.dist_;
}

bool HalfSpaceTest(const Plane &plane, const glm::vec3 &point) {
  // true, the point is in front of the plane
  // false, the point is behind the plane
  return (glm::dot(plane.normal_, point) - plane.dist_ > 0.0f);
}

int TestPlaneSphere(const Sphere &sphere, const Plane &plane) {
  float dist = glm::dot(plane.normal_, sphere.center_) - plane.dist_;
  if (glm::abs(dist) <= sphere.radius_) return kIntersect;
  return dist > sphere.radius_ ? kFront : kBack;
}

int TestPlaneAABB(const AABB &aabb, const Plane &plane) {
  // the largest projected distance (radius) by any vertex of the AABB
  // on the unit normal vector is given by
  // r = abs(Ex*Nx) + abs(Ey*Ny) + abs(Ez*Nz)
  // where "N" is plane's normal and "E" is a box half-width extent
  // extent is always positive:
  float radius = glm::dot(aabb.extent_, glm::abs(plane.normal_));
  // we create second parallel plane with box center and the same normal
  // and distance from box center to plane is simple subtract
  float dist = glm::dot(plane.normal_, aabb.center_) - plane.dist_;
  // if less than radius, return overlap
  if (glm::abs(dist) <= radius) return kIntersect;
  // otherwise it is in front or back of the plane
  return dist > radius ? kFront : kBack;
}

int TestPlaneOBB(const OBB &obb, const Plane &plane) {
  // transpose(mat) * normal is equal to dot product between
  // normal and column.x, column.y, column.z
  // not normalized, part of formula
  glm::vec3 proj_normal = glm::transpose(obb.orientation_) * plane.normal_;
  float radius = glm::dot(obb.extent_, glm::abs(proj_normal));
  float dist = glm::dot(plane.normal_, obb.center_) - plane.dist_;
  if (glm::abs(dist) <= radius) return kIntersect;
  return dist > radius ? kFront : kBack;
}

bool SphereInFrustum(const Sphere &sphere, const Frustum &frustum) {
  for (const auto &plane : frustum.planes_) {
    if (TestPlaneSphere(sphere, plane) == kBack) return false;
  }
  return true;
}

bool AABBInFrustum(const AABB &aabb, const Frustum &frustum) {
  for (const auto &plane : frustum.planes_) {
    if (TestPlaneAABB(aabb, plane) == kBack) return false;
  }
  return true;
}

bool OBBInFrustum(const OBB &obb, const Frustum &frustum) {
  for (const auto &plane : frustum.planes_) {
    if (TestPlaneOBB(obb, plane) == kBack) return false;
  }
  return true;
}

bool AABBInDOP(const AABB &aabb, const DOP &dop) {
  for (unsigned int i = 0; i < dop.total_; ++i) {
    if (TestPlaneAABB(aabb, dop.planes_[i]) == kBack) return false;
  }
  return true;
}

bool OBBInDOP(const OBB &obb, const DOP &dop) {
  for (unsigned int i = 0; i < dop.total_; ++i) {
    if (TestPlaneOBB(obb, dop.planes_[i]) == kBack) return false;
  }
  return true;
}

bool PointInSphere(const Sphere &sphere, const glm::vec3 &point) {
  glm::vec3 delta = point - sphere.center_;
  float dist_sqr = glm::dot(delta, delta);
  return dist_sqr <= sphere.radius_sqr_;
}

bool PointInAABB(const AABB &aabb, const glm::vec3 &point) {
  glm::vec3 min = aabb.Min();
  glm::vec3 max = aabb.Max();
  return (point.x >= min.x && point.x <= max.x) &&
         (point.y >= min.y && point.y <= max.y) &&
         (point.z >= min.z && point.z <= max.z);
}

// distance to all points "P" of a plane
// dot(N, P) = d
// all points "R" of a line can be expressed
// as a point "O" and a vector giving the direction "D"
// R(t) = O + t*D
// with which "t" point "R" contacts the plane?
// dot(N, O + t*D) = d
// dot product can be written as:
//   1. dot(A, B) = len(A)*len(B)*cos(theta)
//   2. dot(A, B) = a1*b1 + a2*b2 + a3*b3
//   3. dot(A, B) = transpose(A)*B
// rewrite equation as Nt * (O + t*D) = d
// Nt*O + Nt*t*D = d
// t = (d - Nt*O) / (Nt*D)
// t = (d - dot(N, O)) / dot(N, D)
bool RayPlane(const Ray &ray, const Plane &plane,
              glm::vec3 &intersection_point) {
  float denominator = glm::dot(plane.normal_, ray.dir_);
  // no intersection, ray is parallel to the plane or lies in plane
  if (glm::abs(denominator) < glm::epsilon<float>()) return false;
  float t = (plane.dist_ - glm::dot(plane.normal_, ray.origin_)) / denominator;
  // no intersection
  if (t < glm::epsilon<float>()) return false;
  intersection_point = ray.origin_ + t * ray.dir_;
  return true;
}

bool RayPlaneDistance(const Ray &ray, const Plane &plane, float &distance) {
  float denominator = glm::dot(plane.normal_, ray.dir_);
  // no intersection, ray is parallel to the plane or lies in plane
  if (glm::abs(denominator) < glm::epsilon<float>()) return false;
  float t = (plane.dist_ - glm::dot(plane.normal_, ray.origin_)) / denominator;
  // no intersection
  if (t < glm::epsilon<float>()) return false;
  distance = t;
  return true;
}

// mix of geometric and analytic solutions
// ray R(t) = O + t*D
// sphere (P - C)^2 - R^2 = 0
// (O + t*D - C)^2 - R^2 = 0
// a*t^2 + b*t + c = 0, where
// a = D*D = 1
// b = 2*D*(O - C)
// c = (O - C)^2 - R^2
bool RaySphereDist(const Sphere &sphere, const Ray &ray, float &t) {
  // swap the sphere's center with ray's origin to omit minus
  glm::vec3 co = sphere.center_ - ray.origin_;
  // float a = glm::dot(ray.dir_, ray.dir_) -> 1, so
  // 2 and 2*a collapse
  float b = glm::dot(co, ray.dir_);
  float c = glm::dot(co, co) - sphere.radius_sqr_;
  float discriminant = b * b - c;
  if (discriminant < 0.0f) {
    t = -1.0f;
    return false;
  }
  t = b - std::sqrt(discriminant);
  return true;
}

bool RaySphere(const Sphere &sphere, const Ray &ray) {
  glm::vec3 co = sphere.center_ - ray.origin_;
  float b = glm::dot(co, ray.dir_);
  float c = glm::dot(co, co) - sphere.radius_sqr_;
  float discriminant = b * b - c;
  return (discriminant >= 0.0f) ? true : false;
}

bool RayAABB(const AABB &aabb, const Ray &ray, float &tnear, float &tfar) {
  // if the ray lies exactly on a slab, we'll have
  // t = 0 * Infinity = NaN
  // GLM has SSE/SSE2 instruction sets, no explicit NaN handling
  glm::vec3 min = aabb.Min();
  glm::vec3 max = aabb.Max();
  float tx1 = (min.x - ray.origin_.x) * ray.dirfrac_.x;
  float tx2 = (max.x - ray.origin_.x) * ray.dirfrac_.x;
  float ty1 = (min.y - ray.origin_.y) * ray.dirfrac_.y;
  float ty2 = (max.y - ray.origin_.y) * ray.dirfrac_.y;
  float tz1 = (min.z - ray.origin_.z) * ray.dirfrac_.z;
  float tz2 = (max.z - ray.origin_.z) * ray.dirfrac_.z;
  float tmin = glm::max(glm::max(glm::min(tx1, tx2), glm::min(ty1, ty2)),
                        glm::min(tz1, tz2));
  float tmax = glm::min(glm::min(glm::max(tx1, tx2), glm::max(ty1, ty2)),
                        glm::max(tz1, tz2));
  tnear = tmin;
  tfar = tmax;
  return tmax >= glm::max(tmin, 0.0f);
}

bool RayAABB(const AABB &aabb, const Ray &ray, float &dist) {
  // if the ray lies exactly on a slab, we'll have
  // t = 0 * Infinity = NaN
  // GLM has SSE/SSE2 instruction sets, no explicit NaN handling
  glm::vec3 min = aabb.Min();
  glm::vec3 max = aabb.Max();
  float tx1 = (min.x - ray.origin_.x) * ray.dirfrac_.x;
  float tx2 = (max.x - ray.origin_.x) * ray.dirfrac_.x;
  float ty1 = (min.y - ray.origin_.y) * ray.dirfrac_.y;
  float ty2 = (max.y - ray.origin_.y) * ray.dirfrac_.y;
  float tz1 = (min.z - ray.origin_.z) * ray.dirfrac_.z;
  float tz2 = (max.z - ray.origin_.z) * ray.dirfrac_.z;
  float tmin = glm::max(glm::max(glm::min(tx1, tx2), glm::min(ty1, ty2)),
                        glm::min(tz1, tz2));
  float tmax = glm::min(glm::min(glm::max(tx1, tx2), glm::max(ty1, ty2)),
                        glm::max(tz1, tz2));
  dist = tmin;
  return tmax >= glm::max(tmin, 0.0f);
}

bool RayOBB(const glm::mat4 &mat, const AABB &aabb, const Ray &ray,
            float &dist) {
  // actually there is no OBB, we convert the ray into AABB's local space
  glm::vec3 scale;
  glm::mat3 rotation;
  glm::vec3 translation;
  math::FastInverse(mat, ray.origin_, scale, rotation, translation);
  // transformation order T*R*S
  glm::vec3 origin{scale * (rotation * translation)};
  glm::vec3 dir{glm::normalize(rotation * ray.dir_)};
  glm::vec3 dirfrac{1.0f / dir};

  // if the ray lies exactly on a slab, we'll have
  // t = 0 * Infinity = NaN
  // GLM has SSE/SSE2 instruction sets, no explicit NaN handling
  glm::vec3 min = aabb.Min();
  glm::vec3 max = aabb.Max();
  float tx1 = (min.x - origin.x) * dirfrac.x;
  float tx2 = (max.x - origin.x) * dirfrac.x;
  float ty1 = (min.y - origin.y) * dirfrac.y;
  float ty2 = (max.y - origin.y) * dirfrac.y;
  float tz1 = (min.z - origin.z) * dirfrac.z;
  float tz2 = (max.z - origin.z) * dirfrac.z;
  float tmin = glm::max(glm::max(glm::min(tx1, tx2), glm::min(ty1, ty2)),
                        glm::min(tz1, tz2));
  float tmax = glm::min(glm::min(glm::max(tx1, tx2), glm::max(ty1, ty2)),
                        glm::max(tz1, tz2));
  dist = tmin;
  return tmax >= glm::max(tmin, 0.0f);
}

bool RayOBB(const OBB &obb, const Ray &ray, float &t) {
  // works only with pure orientation matrix without scale parts
  glm::vec3 dist = obb.center_ - ray.origin_;
  glm::vec3 proj_dist = glm::transpose(obb.orientation_) * dist;
  glm::vec3 proj_dir =
      glm::normalize(glm::transpose(obb.orientation_) * ray.dir_);
  glm::vec3 dirfrac{1.0f / proj_dir};

  // if the ray lies exactly on a slab, we'll have
  // t = 0 * Infinity = NaN
  // GLM has SSE/SSE2 instruction sets, no explicit NaN handling
  float tx1 = (proj_dist.x + obb.extent_.x) * dirfrac.x;
  float tx2 = (proj_dist.x - obb.extent_.x) * dirfrac.x;
  float ty1 = (proj_dist.y + obb.extent_.y) * dirfrac.y;
  float ty2 = (proj_dist.y - obb.extent_.y) * dirfrac.y;
  float tz1 = (proj_dist.z + obb.extent_.z) * dirfrac.z;
  float tz2 = (proj_dist.z - obb.extent_.z) * dirfrac.z;
  float tmin = glm::max(glm::max(glm::min(tx1, tx2), glm::min(ty1, ty2)),
                        glm::min(tz1, tz2));
  float tmax = glm::min(glm::min(glm::max(tx1, tx2), glm::max(ty1, ty2)),
                        glm::max(tz1, tz2));
  t = tmin;
  return tmax >= glm::max(tmin, 0.0f);
}

bool AABBAABB(const AABB &a, const AABB &b) {
  glm::vec3 center = glm::abs(a.center_ - b.center_);
  glm::vec3 extent = a.extent_ + b.extent_;
  if (center.x > extent.x) return false;
  if (center.y > extent.y) return false;
  if (center.z > extent.z) return false;
  // overlap
  return true;
}

bool SphereAABB(const Sphere &sphere, const AABB &aabb) {
  glm::vec3 delta = glm::max(
      glm::vec3(0.0f), glm::abs(aabb.center_ - sphere.center_) - aabb.extent_);
  float dist_sqr = glm::dot(delta, delta);
  return dist_sqr <= sphere.radius_sqr_;
}

bool SphereSphere(const Sphere &a, const Sphere &b) {
  glm::vec3 l = a.center_ - b.center_;
  float sqr_len = glm::dot(l, l);
  float radius_sum = a.radius_ + b.radius_;
  float sqr_radius = radius_sum * radius_sum;
  return sqr_len <= sqr_radius;
}

bool ThreePlanes(const Plane &plane1, const Plane &plane2, const Plane &plane3,
                 glm::vec3 &point) {
  // direction vector for the intersection line L1
  glm::vec3 line_dir1 = glm::cross(plane2.normal_, plane3.normal_);
  float denominator = glm::dot(plane1.normal_, line_dir1);
  // three planes don't intersect at one point
  // but intersect in a line or parallel
  if (glm::abs(denominator) <= glm::epsilon<float>()) return false;
  // signed dist + or -
  point = (line_dir1 * plane1.dist_ +
           glm::cross(plane1.normal_, (plane2.normal_ * plane3.dist_ -
                                       plane3.normal_ * plane2.dist_))) /
          denominator;
  return true;
}

// cross product can be written as:
// 1. cross(A, B) = vec3((y1*z2 - y2*z1), (z1*x2 - z2*x1), (x1*y2 - x2*y1))
// 2. len(cross(A, B)) = len(A)*len(B)*sin(theta) = area
// 3. cross(A, B) = det(A, B) = volume
bool ThreePlanesDeterminant(const Plane &plane1, const Plane &plane2,
                            const Plane &plane3, glm::vec3 &point) {
  // fixed for frustum
  float determinant = glm::determinant(
      glm::mat3(plane1.normal_, plane2.normal_, plane3.normal_));
  // parallel planes
  if (determinant <= glm::epsilon<float>()) return false;
  // signed dist + or -
  glm::vec3 line_dir1 =
      glm::cross(plane2.normal_, plane3.normal_) * plane1.dist_;
  glm::vec3 line_dir2 =
      glm::cross(plane3.normal_, plane1.normal_) * plane2.dist_;
  glm::vec3 line_dir3 =
      glm::cross(plane1.normal_, plane2.normal_) * plane3.dist_;
  point = (line_dir1 + line_dir2 + line_dir3) / determinant;
  return true;
}

}  // namespace math