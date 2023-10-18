#pragma once

// deps
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// global
#include <random>
// local
#include "mi_types.h"

namespace rnd {

std::mt19937 &GetPRNG();

// [0.0f - 1.0f]
float GetDefaultFloat();
float GetFloat(float min, float max);
glm::vec3 GetDefaultVec3();
// [0.0f - 2PI]
float GetTwoPiFloat();

glm::vec3 Vec3(float x_min, float x_max, float y_min, float y_max, float z_min,
               float z_max);
float AngleRad(float min_deg, float max_deg);

// gen tex
void GenerateSampleKernel(int kernel_size, MiVector<glm::vec3> &kernel);
void GenerateNoise(int noise_size, MiVector<glm::vec3> &noise);
void GenerateRandomHbao(int random_elements,
                        MiVector<glm::vec4> &random_uniform);
void GenerateShadowAngles(unsigned int size,
                          MiVector<glm::vec2> &random_angles);
void GenerateHilbertIndices(const uint32_t size, MiVector<uint16_t> &indices);

}  // namespace rnd