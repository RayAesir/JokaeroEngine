#pragma once

// deps
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace assglm {

inline glm::mat4 GetMatrix(const aiMatrix4x4& from) {
  return glm::transpose(glm::make_mat4(&from.a1));
}

inline glm::vec3 GetVec(const aiVector3D& vec) {
  return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::quat GetQuat(const aiQuaternion& pOrientation) {
  return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y,
                   pOrientation.z);
}

}  // namespace assglm
