#pragma once

// deps
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace math {

void FastDecomposeMat4(const glm::mat4 &mat, glm::vec3 &scale,
                       glm::mat3 &rotation, glm::vec3 &translation);
void FastDecomposeMat3(const glm::mat3 &mat, glm::vec3 &scale,
                       glm::mat3 &rotation);
void FastInverse(const glm::mat4 &mat, const glm::vec3 &pos, glm::vec3 &scale,
                 glm::mat3 &rotation, glm::vec3 &translation);

}  // namespace math