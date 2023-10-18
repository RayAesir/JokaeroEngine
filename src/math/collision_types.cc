#include "collision_types.h"

// deps
#include <glm/gtc/matrix_access.hpp>
// local
#include "math/transformation.h"

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &dir)
    : origin_(origin), dir_(glm::normalize(dir)), dirfrac_(1.0f / dir) {}

Plane::Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) {
  glm::vec3 u = p1 - p0;
  glm::vec3 v = p2 - p0;
  normal_ = glm::normalize(glm::cross(u, v));
  dist_ = glm::dot(normal_, p0);
}

Plane::Plane(const glm::vec3 &p0, const glm::vec3 &normal) : normal_(normal) {
  dist_ = glm::dot(normal_, p0);
}

Plane::Plane(const float &dist, const glm::vec3 &normal)
    : normal_(normal), dist_(dist) {}

AABB::AABB(const MiVector<glm::vec3> &vertices) {
  glm::vec3 min = GetMaxLimits<glm::vec3, float>();
  glm::vec3 max = GetMinLimits<glm::vec3, float>();
  for (const auto &v : vertices) {
    min = glm::min(min, v);
    max = glm::max(max, v);
  }
  center_ = (max + min) * 0.5f;
  extent_ = max - center_;
}

AABB::AABB(const Corners &corners) {
  glm::vec3 min = GetMaxLimits<glm::vec3, float>();
  glm::vec3 max = GetMinLimits<glm::vec3, float>();
  for (const auto &c : corners) {
    min = glm::min(min, c);
    max = glm::max(max, c);
  }
  center_ = (max + min) * 0.5f;
  extent_ = max - center_;
}

AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) {
  center_ = (max + min) * 0.5f;
  extent_ = max - center_;
}

glm::vec3 AABB::Min() const { return center_ - extent_; }

glm::vec3 AABB::Max() const { return center_ + extent_; }

void AABB::Transform(const AABB &bb, const glm::mat4 &mat) {
  // From local-space to world-space
  // The basic method is to apply matrix to all 8 points of the AABB
  // and find min, max again
  // But instead we can use centroid box notation
  glm::mat3 abs_mat{mat};
  abs_mat[0] = glm::abs(abs_mat[0]);
  abs_mat[1] = glm::abs(abs_mat[1]);
  abs_mat[2] = glm::abs(abs_mat[2]);
  center_ = mat * glm::vec4(bb.center_, 1.0f);
  extent_ = abs_mat * bb.extent_;
}

AABB AABB::Transform(const glm::mat4 &mat) const {
  glm::mat3 abs_mat{mat};
  abs_mat[0] = glm::abs(abs_mat[0]);
  abs_mat[1] = glm::abs(abs_mat[1]);
  abs_mat[2] = glm::abs(abs_mat[2]);
  AABB bb;
  bb.center_ = mat * glm::vec4(center_, 1.0f);
  bb.extent_ = abs_mat * extent_;
  return bb;
}

void AABB::Move(const glm::vec3 &pos) { center_ += pos; }

void AABB::MoveAndResize(const glm::vec3 &pos, const glm::vec3 &size) {
  center_ += pos;
  extent_ += size;
}

Corners AABB::GetCorners() const {
  using enum FrustumPoints::Enum;
  const glm::vec3 &c = center_;
  const glm::vec3 &e = extent_;
  Corners corners;
  corners[kFarBottomLeft] = glm::vec3(c.x + e.x, c.y + e.y, c.z + e.z);
  corners[kFarTopLeft] = glm::vec3(c.x - e.x, c.y + e.y, c.z + e.z);
  corners[kFarTopRight] = glm::vec3(c.x + e.x, c.y - e.y, c.z + e.z);
  corners[kFarBottomRight] = glm::vec3(c.x + e.x, c.y + e.y, c.z - e.z);
  corners[kNearBottomLeft] = glm::vec3(c.x - e.x, c.y - e.y, c.z + e.z);
  corners[kNearTopLeft] = glm::vec3(c.x + e.x, c.y - e.y, c.z - e.z);
  corners[kNearTopRight] = glm::vec3(c.x - e.x, c.y + e.y, c.z - e.z);
  corners[kNearBottomRight] = glm::vec3(c.x - e.x, c.y - e.y, c.z - e.z);
  return corners;
}

void AABB::ExpandToInclude(const glm::vec3 &point) {
  glm::vec3 min = glm::min(Min(), point);
  glm::vec3 max = glm::max(Max(), point);
  center_ = (max + min) * 0.5f;
  extent_ = max - center_;
}

void AABB::ExpandToInclude(const AABB &aabb) {
  glm::vec3 min = glm::min(Min(), aabb.Min());
  glm::vec3 max = glm::max(Max(), aabb.Max());
  center_ = (max + min) * 0.5f;
  extent_ = max - center_;
}

float AABB::SurfaceArea() const {
  return 2.0f * ((extent_.x * extent_.z) + (extent_.x * extent_.y) +
                 (extent_.y * extent_.z));
}

// return x as 0, y as 1, z as 2
unsigned int AABB::MaxDimension() const {
  // assume x axis is longest first
  unsigned int result = 0;
  if (extent_.y > extent_.x) result = 1;
  if (extent_.z > extent_.x) result = 2;
  return result;
}

OBB::OBB(const AABB &aabb, const Transformation &t) { Transform(aabb, t); }

void OBB::Transform(const AABB &aabb, const Transformation &t) {
  extent_ = t.GetScale() * aabb.extent_;
  // some BBs from game meshes have offset (bone system)
  // to fix this: scale -> rotation -> translation
  center_ = (t.GetQuat() * (t.GetScale() * aabb.center_)) + t.GetPosition();
  orientation_ = t.GetRotation();
}

Sphere::Sphere(const glm::vec3 &center, float radius)
    : center_(center), radius_(radius), radius_sqr_(radius * radius) {}

Sphere::Sphere(const AABB &aabb) : center_(aabb.center_) {
  radius_ = glm::length(aabb.extent_);
  radius_sqr_ = radius_ * radius_;
}

Corners Sphere::GetCorners() const {
  using enum FrustumPoints::Enum;
  const glm::vec3 &c = center_;
  float r = radius_;
  Corners corners;
  corners[kFarBottomLeft] = glm::vec3(c.x + r, c.y + r, c.z + r);
  corners[kFarTopLeft] = glm::vec3(c.x - r, c.y + r, c.z + r);
  corners[kFarTopRight] = glm::vec3(c.x + r, c.y - r, c.z + r);
  corners[kFarBottomRight] = glm::vec3(c.x + r, c.y + r, c.z - r);
  corners[kNearBottomLeft] = glm::vec3(c.x - r, c.y - r, c.z + r);
  corners[kNearTopLeft] = glm::vec3(c.x + r, c.y - r, c.z - r);
  corners[kNearTopRight] = glm::vec3(c.x - r, c.y + r, c.z - r);
  corners[kNearBottomRight] = glm::vec3(c.x - r, c.y - r, c.z - r);
  return corners;
}

// Gribb/Hartmann method
// the input matrix can be: proj -> view space or proj_view -> world space
Frustum::Frustum(const glm::mat4 &mat) {
  using enum FrustumPlanes::Enum;
  glm::vec4 row_x = glm::row(mat, 0);
  glm::vec4 row_y = glm::row(mat, 1);
  glm::vec4 row_z = glm::row(mat, 2);
  glm::vec4 row_w = glm::row(mat, 3);
  glm::vec4 planes[FrustumPlanes::kTotal];
  planes[kLeft] = row_w + row_x;
  planes[kRight] = row_w - row_x;
  planes[kBottom] = row_w + row_y;
  planes[kTop] = row_w - row_y;
  planes[kNear] = row_w + row_z;
  planes[kFar] = row_w - row_z;
  // normalizing the planes
  for (unsigned int p = 0; p < FrustumPlanes::kTotal; ++p) {
    planes[p] = planes[p] / glm::length(glm::vec3(planes[p]));
    planes_[p] = Plane(-planes[p].w, glm::vec3(planes[p]));
  }
}

glm::uvec4 DOP::GetNeighbors(unsigned int plane) const {
  using enum FrustumPlanes::Enum;
  static constexpr glm::uvec4 table[FrustumPlanes::kTotal] = {
      // left
      {kTop, kBottom, kNear, kFar},
      // right
      {kTop, kBottom, kNear, kFar},
      // top
      {kLeft, kRight, kNear, kFar},
      // bottom
      {kLeft, kRight, kNear, kFar},
      // near
      {kLeft, kRight, kTop, kBottom},
      // far
      {kLeft, kRight, kTop, kBottom},
  };
  return table[plane];
}

glm::uvec2 DOP::GetCornersOfPlanes(unsigned int plane1,
                                   unsigned int plane2) const {
  using enum FrustumPoints::Enum;
  static constexpr int table[FrustumPlanes::kTotal][FrustumPlanes::kTotal][2] =
      {
          // left
          {
              // left // Invalid combination.
              {
                  kFarBottomLeft,
                  kFarBottomLeft,
              },
              // right  // Invalid combination.
              {
                  kFarBottomLeft,
                  kFarBottomLeft,
              },
              // top
              {
                  kNearTopLeft,
                  kFarTopLeft,
              },
              // bottom
              {
                  kFarBottomLeft,
                  kNearBottomLeft,
              },
              // near
              {
                  kNearBottomLeft,
                  kNearTopLeft,
              },
              // far
              {
                  kFarTopLeft,
                  kFarBottomLeft,
              },
          },
          // right
          {
              // left  // Invalid combination.
              {
                  kFarBottomRight,
                  kFarBottomRight,
              },
              // right  // Invalid combination.
              {
                  kFarBottomRight,
                  kFarBottomRight,
              },
              // top
              {
                  kFarTopRight,
                  kNearTopRight,
              },
              // bottom
              {
                  kNearBottomRight,
                  kFarBottomRight,
              },
              // near
              {
                  kNearTopRight,
                  kNearBottomRight,
              },
              // far
              {
                  kFarBottomRight,
                  kFarTopRight,
              },
          },
          // top
          {
              // left
              {
                  kFarTopLeft,
                  kNearTopLeft,
              },
              // right
              {
                  kNearTopRight,
                  kFarTopRight,
              },
              // top  // Invalid combination.
              {
                  kNearTopLeft,
                  kFarTopLeft,
              },
              // bottom  // Invalid combination.
              {
                  kFarBottomLeft,
                  kNearBottomLeft,
              },
              // near
              {
                  kNearTopLeft,
                  kNearTopRight,
              },
              // far
              {
                  kFarTopRight,
                  kFarTopLeft,
              },
          },
          // bottom
          {
              // left
              {
                  kNearBottomLeft,
                  kFarBottomLeft,
              },
              // right
              {
                  kFarBottomRight,
                  kNearBottomRight,
              },
              // top  // Invalid combination.
              {
                  kNearBottomLeft,
                  kFarBottomLeft,
              },
              // bottom  // Invalid combination.
              {
                  kFarBottomLeft,
                  kNearBottomLeft,
              },
              // near
              {
                  kNearBottomRight,
                  kNearBottomLeft,
              },
              // far
              {
                  kFarBottomLeft,
                  kFarBottomRight,
              },
          },
          // near
          {
              // left
              {
                  kNearTopLeft,
                  kNearBottomLeft,
              },
              // right
              {
                  kNearBottomRight,
                  kNearTopRight,
              },
              // top
              {
                  kNearTopRight,
                  kNearTopLeft,
              },
              // bottom
              {
                  kNearBottomLeft,
                  kNearBottomRight,
              },
              // near  // Invalid combination.
              {
                  kNearTopLeft,
                  kNearTopRight,
              },
              // far  // Invalid combination.
              {
                  kFarTopRight,
                  kFarTopLeft,
              },
          },
          // far
          {
              // left
              {
                  kFarBottomLeft,
                  kFarTopLeft,
              },
              // right
              {
                  kFarTopRight,
                  kFarBottomRight,
              },
              // top
              {
                  kFarTopLeft,
                  kFarTopRight,
              },
              // bottom
              {
                  kFarBottomRight,
                  kFarBottomLeft,
              },
              // near  // Invalid combination.
              {
                  kFarTopLeft,
                  kFarTopRight,
              },
              // far  // Invalid combination.
              {
                  kFarTopRight,
                  kFarTopLeft,
              },
          },

      };
  return glm::uvec2(table[plane1][plane2][0], table[plane1][plane2][1]);
};

DOP::DOP(const Frustum &frustum, const Corners &corners,
         const glm::vec3 &light_dir) {
  // add planes that are facing towards us (light)
  for (unsigned int p = 0; p < FrustumPlanes::kTotal; ++p) {
    float dir = glm::dot(frustum.planes_[p].normal_, light_dir);
    if (dir < glm::epsilon<float>()) {
      planes_[total_] = frustum.planes_[p];
      ++total_;
    }
  }
  // find the edges
  for (unsigned int p = 0; p < FrustumPlanes::kTotal; ++p) {
    // if plane is facing away from us, move on
    float dir = glm::dot(frustum.planes_[p].normal_, light_dir);
    if (dir > glm::epsilon<float>()) continue;
    // for each neighbor of this plane
    glm::uvec4 neighbors = GetNeighbors(p);
    for (unsigned int n = 0; n < 4U; ++n) {
      float neighbor_dir =
          glm::dot(frustum.planes_[neighbors[n]].normal_, light_dir);
      // if this plane is facing away form us,
      // the edge between plane "p" and plane "n"
      if (neighbor_dir > glm::epsilon<float>()) {
        glm::uvec2 edge = GetCornersOfPlanes(p, neighbors[n]);
        Plane add_plane{corners[edge[0]], corners[edge[1]],
                        corners[edge[0]] - light_dir};
        planes_[total_] = add_plane;
        ++total_;
      }
    }
  }
}