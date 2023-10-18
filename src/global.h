#pragma once

// deps
#include <glad/glad.h>

#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

template <typename X, typename Y>
constexpr X CeilInt(X x, Y y) {
  return (x + static_cast<X>(y) - static_cast<X>(1)) / static_cast<X>(y);
};

struct State {
  enum Enum : uint8_t { kLoading, kScene, kTotal };
};

// order is important to bind correct Program/VAO
struct MeshType {
  enum Enum : GLuint {
    kStaticOpaque,
    kSkinnedOpaque,
    kStaticAlphaClip,
    kSkinnedAlphaClip,
    kStaticAlphaBlend,
    kSkinnedAlphaBlend,
    kTotal
  };
  static constexpr GLuint kNoAlphaBlend = Enum::kStaticAlphaBlend;
};

// frustum culling
struct RenderPass {
  enum Enum : GLuint {
    kDirectShadowPass,
    kPointShadowPass,
    kViewPass,
    kViewOcclusionPass,
    kTotal
  };
};

namespace global {

inline State::Enum gState = State::Enum::kLoading;

// assume that all Objects have single Mesh (Instance)
// sizeof(glm::mat4) == 64 bytes, 16mb of RAM == 256k instances
inline constexpr GLuint kMaxInstances = 256 * 1024;
// equal to number of meshes, 16k meshes in the batch
inline constexpr GLuint kMaxDrawCommands = 16 * 1024;
inline constexpr const char* kEmptyName{"none"};
// 4kk * sizeof(Static) == 4kk * 56 = 224mb
// 16kk * sizeof(Skinned) == 4kk * 88 = 352mb
inline constexpr GLuint kMaxVerticesPerMemBlock = 4 * 1024 * 1024;
// 4kk * sizeof(GL_UNSIGNED_INT) == 16mb
inline constexpr GLuint kMaxIndicesPerMemBlock = 4 * 1024 * 1024;
// animation parameters, max bones defined by assimp is 4
inline constexpr GLint kMaxBonesPerVertex = 4;
inline constexpr GLint kMaxAnimatedActors = 128;
// assume that a model has at the average 128 bones
inline constexpr GLint kMaxBoneMatrices = 128 * kMaxAnimatedActors;

// engine light
inline constexpr GLuint kMaxPointLights = 1024;
inline constexpr GLuint kMaxPointShadows = 32;
inline constexpr GLuint kMaxPointShadowsPerInstance = 64;
// frustum clusters, aspect ratio and 24 Z slices
inline constexpr glm::uvec3 kFrustumClusters{16, 9, 24};
inline constexpr glm::uvec3 kFrustumClustersThreads{16, 9, 4};
inline constexpr GLuint kFrustumClustersCount =
    kFrustumClusters.x * kFrustumClusters.y * kFrustumClusters.z;
inline constexpr GLuint kMaxPointLightsPerCluster = 64;
inline constexpr GLuint kCulledPointLights =
    kFrustumClustersCount * kMaxPointLightsPerCluster;
// compute shaders for stream compaction (prefix sum)
// 512x512 = 262144 kMaxInstances
inline constexpr GLuint kThreadsPerGroup = 512;
// engine shadows
inline constexpr GLuint kCubemapFaces = 6;
inline constexpr GLuint kFrustumPlanes = 6;
inline constexpr GLuint kDopPlanes = 11;
inline constexpr GLuint kMaxShadowCascades = 4;
inline constexpr GLfloat kSplitWeight = 0.75f;

// engine particles
inline constexpr GLuint kMaxFxPresets = 16;
// buffer size for all FX (sum of num_of_particles)
inline constexpr GLuint kMaxParticlesNum = 2048;
inline constexpr GLuint kMaxParticlesBufferSize =
    kMaxFxPresets * kMaxParticlesNum;
inline constexpr GLuint kMaxFxInstances = 128;

// generated mesh settings
inline constexpr GLuint kSlices = 40;
inline constexpr GLuint kStacks = 20;
inline constexpr GLfloat kHalfRadius = 0.5f;
inline constexpr GLfloat kFullRadius = 1.0f;
inline constexpr GLfloat kHeight = 1.0f;
inline constexpr GLuint kPlaneSlicesStacks = 10;

// readback buffer to debug shaders
inline constexpr GLuint kDebugReadbackSize = 32;
// 8 point to represent camera, 32 cameras
inline constexpr GLuint kDebugCamerasPoints = 8 * 32;

// transformation
inline constexpr glm::vec3 kWorldRight{1.0f, 0.0f, 0.0f};
inline constexpr glm::vec3 kWorldUp{0.0f, 1.0f, 0.0f};
inline constexpr glm::vec3 kWorldFront{0.0f, 0.0f, -1.0f};

// camera, vertical fov
inline constexpr GLfloat kMinFovDeg = 5.0f;
inline constexpr GLfloat kMaxFovDeg = 90.0f;
inline constexpr GLfloat kMinFov{glm::radians(kMinFovDeg)};
inline constexpr GLfloat kMaxFov{glm::radians(kMaxFovDeg)};
inline constexpr GLfloat kDefaultFov{glm::radians(60.0f)};

// shadows PCF
inline constexpr GLuint kRandomAnglesSize = 128;
inline constexpr GLuint kPoissonMaxSize = 128;

// HBAO
inline constexpr GLint kRandomSize = 4;
inline constexpr GLint kRandomElements = kRandomSize * kRandomSize;

// GTAO
inline constexpr GLint kHilbertLevel = 6;
inline constexpr GLint kHilbertSize = 1 << 6;

// framebuffer
inline constexpr GLfloat kClearBlack[4]{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr GLfloat kClearBlackAlphaOne[4]{0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr GLfloat kClearWhite[4]{1.0f, 1.0f, 1.0f, 1.0f};
// default depth and reversed-z
inline constexpr GLfloat kClearReverseDepth = 0.0f;
inline constexpr GLfloat kClearDepth = 1.0f;
// clean buffers with 32 zero bits
inline constexpr GLuint kZero4Bytes = 0;
// bloom, enough for 8k resolution
inline constexpr GLint kBloomDownscaleLimit = 10;

// colors
inline constexpr glm::vec3 kRed{1.0f, 0.0f, 0.0f};
inline constexpr glm::vec3 kGreen{0.0f, 1.0f, 0.0f};
inline constexpr glm::vec3 kBlue{0.0f, 0.0f, 1.0f};
inline constexpr glm::vec3 kYellow{1.0f, 1.0f, 0.0f};
inline constexpr glm::vec3 kCyan{0.0f, 1.0f, 1.0f};
inline constexpr glm::vec3 kPurple{1.0f, 0.0f, 1.0f};
inline constexpr glm::vec3 kBlack{0.0f, 0.0f, 0.0f};
inline constexpr glm::vec3 kWhite{1.0f, 1.0f, 1.0f};

}  // namespace global
