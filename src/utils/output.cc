#include "output.h"

std::ostream &operator<<(std::ostream &os, const Ray &ray) {
  os << fmt::format("Ray: [origin: {:.3f}, dir: {:.3f}]\n", ray.origin_,
                    ray.dir_);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Plane &plane) {
  os << fmt::format("Plane: [dist: {:.3f}, normal: {:.3f}]\n", plane.dist_,
                    plane.normal_);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Sphere &sphere) {
  os << fmt::format("Sphere: [center: {:.3f}, radius: {:.3f}]\n",
                    sphere.center_, sphere.radius_);
  return os;
}

std::ostream &operator<<(std::ostream &os, const AABB &aabb) {
  os << fmt::format("AABB: [center: {:.3f}, extent: {:.3f}]\n", aabb.center_,
                    aabb.extent_);
  return os;
}

std::ostream &operator<<(std::ostream &os, const OBB &obb) {
  os << fmt::format("OBB: [center: {:.3f}, extent: {:.3f}]\n", obb.center_,
                    obb.extent_);
  os << fmt::format("rotation: {:.3f}\n", obb.orientation_);
  return os;
}
