#include "random.h"

// deps
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
// global
#include <algorithm>
#include <array>

namespace rnd {

namespace {
// - the performance of std::random_device degrades sharply
// once the entropy pool is exhausted, can throw
// - for practical use std::random_device is generally only used to seed
// a PRNG such as std::mt19937
// - the generator should be initialised only once (static) for each thread
// - the distribution objects are cheap
std::mt19937 gPRNG = ([]() {
  std::array<int, std::mt19937::state_size> seed_data;
  std::random_device rd;
  std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
  std::seed_seq seed_seq(std::begin(seed_data), std::end(seed_data));
  return std::mt19937(seed_seq);
})();

std::uniform_real_distribution<float> gRandomFloats{0.0f, 1.0f};
std::uniform_real_distribution<float> gRandomTwoPi{0.0f, glm::two_pi<float>()};
}  // namespace

std::mt19937 &GetPRNG() { return gPRNG; }

float GetDefaultFloat() { return gRandomFloats(gPRNG); }

float GetFloat(float min, float max) {
  return glm::clamp(max * gRandomFloats(gPRNG), min, max);
}

float GetTwoPiFloat() { return gRandomTwoPi(gPRNG); }

glm::vec3 GetDefaultVec3() {
  return glm::vec3(rnd::GetDefaultFloat(), rnd::GetDefaultFloat(),
                   rnd::GetDefaultFloat());
}

glm::vec3 Vec3(float x_min, float x_max, float y_min, float y_max, float z_min,
               float z_max) {
  std::uniform_real_distribution<float> dist_x(x_min, x_max);
  std::uniform_real_distribution<float> dist_y(y_min, y_max);
  std::uniform_real_distribution<float> dist_z(z_min, z_max);
  return glm::vec3(dist_x(gPRNG), dist_y(gPRNG), dist_z(gPRNG));
}

// angle in radians
float AngleRad(float min_deg, float max_deg) {
  std::uniform_real_distribution<float> angle(min_deg, max_deg);
  return glm::radians(angle(gPRNG));
}

// generate sample kernel (hemisphere) e.g. 8x8px total 64px
void GenerateSampleKernel(int kernel_elements, MiVector<glm::vec3> &kernel) {
  kernel.reserve(kernel_elements);

  auto lerp = [](float a, float b, float f) -> float {
    return a + f * (b - a);
  };

  for (int i = 0; i < kernel_elements; ++i) {
    glm::vec3 sample(GetDefaultFloat() * 2.0f - 1.0f,
                     GetDefaultFloat() * 2.0f - 1.0f, GetDefaultFloat());
    sample = glm::normalize(sample);
    sample *= GetDefaultFloat();
    // scale samples [s, t] they're more aligned to center of kernel
    float scale = static_cast<float>(i) / static_cast<float>(kernel_elements);
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    kernel.push_back(sample);
  }
}

// generate noise texture e.g. 4x4px total 16px
void GenerateNoise(int noise_elements, MiVector<glm::vec3> &noise) {
  noise.reserve(noise_elements);
  for (int i = 0; i < noise_elements; ++i) {
    // rotate around z-axis in tangent space
    glm::vec3 sample(GetDefaultFloat() * 2.0f - 1.0f,
                     GetDefaultFloat() * 2.0f - 1.0f, 0.0f);
    noise.push_back(sample);
  }
}

// generate sample kernel
void GenerateRandomHbao(int random_elements,
                        MiVector<glm::vec4> &random_uniform) {
  random_uniform.reserve(random_elements);
  // set in shader too
  constexpr float num_dir{8};
  for (int i = 0; i < random_elements; ++i) {
    float rand1 = GetDefaultFloat();
    float rand2 = GetDefaultFloat();
    // use random rotation angles in [0.2PI / num_dir]
    float angle = glm::two_pi<float>() * rand1 / num_dir;
    random_uniform.emplace_back(glm::cos(angle), glm::sin(angle), rand2, 0.0f);
  }
}

void GenerateShadowAngles(unsigned int size,
                          MiVector<glm::vec2> &random_angles) {
  random_angles.resize(size * size * size);
  for (unsigned int z = 0; z < size; ++z) {
    for (unsigned int y = 0; y < size; ++y) {
      for (unsigned int x = 0; x < size; ++x) {
        unsigned int index = x + y * size + z * size * size;
        float random_angle = GetTwoPiFloat();
        random_angles[index] =
            glm::vec2{glm::cos(random_angle), glm::sin(random_angle)};
      }
    }
  }
}

void GenerateHilbertIndices(const uint32_t size, MiVector<uint16_t> &indices) {
  auto hilbert_index = [&size](uint32_t pos_x, uint32_t pos_y) -> uint32_t {
    uint32_t index = 0U;
    for (uint32_t curLevel = size / 2U; curLevel > 0U; curLevel /= 2U) {
      uint32_t regionX = static_cast<uint32_t>((pos_x & curLevel) > 0U);
      uint32_t regionY = static_cast<uint32_t>((pos_y & curLevel) > 0U);
      index += curLevel * curLevel * ((3U * regionX) ^ regionY);
      if (regionY == 0U) {
        if (regionX == 1U) {
          pos_x = (size - 1U) - pos_x;
          pos_y = (size - 1U) - pos_y;
        }

        uint32_t temp = pos_x;
        pos_x = pos_y;
        pos_y = temp;
      }
    }
    return index;
  };

  indices.resize(size * size);

  for (uint32_t x = 0; x < size; ++x) {
    for (uint32_t y = 0; y < size; ++y) {
      uint32_t index = hilbert_index(x, y);
      indices[x + size * y] = static_cast<uint16_t>(index);
    }
  }
}

}  // namespace rnd
