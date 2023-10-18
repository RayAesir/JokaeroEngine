#pragma once

// local
#include "math/collision_types.h"

namespace math {

enum PlaneIntersect : int { kFront, kBack, kIntersect };

float GetSignedDistance(const Plane &plane, const glm::vec3 &point);
bool HalfSpaceTest(const Plane &plane, const glm::vec3 &point);

int TestPlaneSphere(const Sphere &sphere, const Plane &plane);
int TestPlaneAABB(const AABB &aabb, const Plane &plane);
int TestPlaneOBB(const OBB &obb, const Plane &plane);

bool SphereInFrustum(const Sphere &sphere, const Frustum &frustum);
bool AABBInFrustum(const AABB &aabb, const Frustum &frustum);
bool OBBInFrustum(const OBB &obb, const Frustum &frustum);

bool AABBInDOP(const AABB &aabb, const DOP &dop);
bool OBBInDOP(const OBB &obb, const DOP &dop);

bool PointInSphere(const Sphere &sphere, const glm::vec3 &point);
bool PointInAABB(const AABB &aabb, const glm::vec3 &point);

bool RayPlane(const Ray &ray, const Plane &plane,
              glm::vec3 &intersection_point);
bool RayPlaneDistance(const Ray &ray, const Plane &plane, float &distance);

bool RaySphereDist(const Sphere &sphere, const Ray &ray, float &t);
bool RaySphere(const Sphere &sphere, const Ray &ray);
bool RayAABB(const AABB &aabb, const Ray &ray, float &dist);
bool RayAABB(const AABB &aabb, const Ray &ray, float &tnear, float &tfar);
bool RayOBB(const glm::mat4 &mat, const AABB &aabb, const Ray &ray,
            float &dist);
bool RayOBB(const OBB &obb, const Ray &ray, float &t);

bool AABBAABB(const AABB &a, const AABB &b);
bool SphereAABB(const Sphere &sphere, const AABB &aabb);
bool SphereSphere(const Sphere &a, const Sphere &b);

bool ThreePlanes(const Plane &plane1, const Plane &plane2, const Plane &plane3,
                 glm::vec3 &point);
bool ThreePlanesDeterminant(const Plane &plane1, const Plane &plane2,
                            const Plane &plane3, glm::vec3 &point);

}  // namespace math