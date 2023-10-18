#include "fast_math.h"

namespace math {

void FastDecomposeMat4(const glm::mat4 &mat, glm::vec3 &scale,
                       glm::mat3 &rotation, glm::vec3 &translation) {
  // matrix decompose, only for uniform scaled
  // S
  float scale_x = glm::length(glm::vec3(mat[0]));
  float scale_y = glm::length(glm::vec3(mat[1]));
  float scale_z = glm::length(glm::vec3(mat[2]));
  scale = glm::vec3(scale_x, scale_y, scale_z);
  // R
  glm::mat3 tmp{mat};
  rotation =
      glm::mat3((tmp[0] / scale_x), (tmp[1] / scale_y), (tmp[2] / scale_z));
  // T
  translation = glm::vec3(mat[3]);
}

void FastDecomposeMat3(const glm::mat3 &mat, glm::vec3 &scale,
                       glm::mat3 &rotation) {
  // matrix decompose, only for uniform scaled
  // S
  float scale_x = glm::length(mat[0]);
  float scale_y = glm::length(mat[1]);
  float scale_z = glm::length(mat[2]);
  scale = glm::vec3(scale_x, scale_y, scale_z);
  // R
  rotation =
      glm::mat3((mat[0] / scale_x), (mat[1] / scale_y), (mat[2] / scale_z));
}

void FastInverse(const glm::mat4 &mat, const glm::vec3 &pos, glm::vec3 &scale,
                 glm::mat3 &rotation, glm::vec3 &translation) {
  // matrix decompose, only for uniform scaled
  // S
  float inv_scale_x = 1.0f / glm::length(glm::vec3(mat[0]));
  float inv_scale_y = 1.0f / glm::length(glm::vec3(mat[1]));
  float inv_scale_z = 1.0f / glm::length(glm::vec3(mat[2]));
  scale = glm::vec3(inv_scale_x, inv_scale_y, inv_scale_z);
  // R
  glm::mat3 tmp{glm::transpose(mat)};
  rotation = glm::mat3((tmp[0] * inv_scale_x), (tmp[1] * inv_scale_y),
                       (tmp[2] * inv_scale_z));
  // T
  translation = pos - glm::vec3(mat[3]);
}

}  // namespace math