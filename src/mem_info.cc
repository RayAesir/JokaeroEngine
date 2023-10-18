#include "mem_info.h"

// global
#include <array>
#include <mutex>
// local
#include "mi_types.h"

namespace mem {

namespace {
std::array<MiUnMap<const void*, GLuint64>, kTotal> gMaps;
std::mutex gMutex;
}  // namespace

void Add(Type type, const void* obj, GLuint64 bytes) {
  std::scoped_lock lock(gMutex);
  gMaps[type].try_emplace(obj, bytes);
}

void Erase(Type type, const void* obj) {
  std::scoped_lock lock(gMutex);
  auto& map = gMaps[type];
  if (map.contains(obj)) {
    map.erase(obj);
  }
}

void Move(Type type, const void* other_obj, const void* this_obj) {
  std::scoped_lock lock(gMutex);
  auto& map = gMaps[type];
  auto it = map.find(other_obj);
  if (it != map.end()) {
    size_t bytes = it->second;
    map.try_emplace(this_obj, bytes);
  }
}

enum TexSpace : int { k1D, k2D, k3D };

GLuint64 MipMap(TexSpace space, GLuint64 size) {
  // fast approximation
  switch (space) {
    case TexSpace::k1D:
      return 15 * size / 10;  // 50%
    case TexSpace::k2D:
      return 133 * size / 100;  // 33%
    case TexSpace::k3D:
      return 125 * size / 100;  // 25%
    default:
      return size;
  }
}

GLuint64 SizeOfFormat(int internal_format) {
  // max to byte (approximation)
  switch (internal_format) {
    case GL_R8:
      return 1;
    case GL_R8_SNORM:
      return 1;
    case GL_R16:
      return 2;
    case GL_R16_SNORM:
      return 2;
    case GL_RG8:
      return 2;
    case GL_RG8_SNORM:
      return 2;
    case GL_RG16:
      return 4;
    case GL_RG16_SNORM:
      return 4;
    case GL_R3_G3_B2:
      return 1;
    case GL_RGB4:
      return 2;
    case GL_RGB5:
      return 2;
    case GL_RGB8:
      return 3;
    case GL_RGB8_SNORM:
      return 3;
    case GL_RGB10:
      return 4;
    case GL_RGB12:
      return 5;
    case GL_RGB16_SNORM:
      return 6;
    case GL_RGBA2:
      return 1;
    case GL_RGBA4:
      return 2;
    case GL_RGB5_A1:
      return 2;
    case GL_RGBA8:
      return 4;
    case GL_RGBA8_SNORM:
      return 4;
    case GL_RGB10_A2:
      return 4;
    case GL_RGB10_A2UI:
      return 4;
    case GL_RGBA12:
      return 6;
    case GL_RGBA16:
      return 8;
    case GL_SRGB8:
      return 3;
    case GL_SRGB8_ALPHA8:
      return 4;
    case GL_R16F:
      return 2;
    case GL_RG16F:
      return 4;
    case GL_RGB16F:
      return 6;
    case GL_RGBA16F:
      return 8;
    case GL_R32F:
      return 4;
    case GL_RG32F:
      return 8;
    case GL_RGB32F:
      return 12;
    case GL_RGBA32F:
      return 16;
    case GL_R11F_G11F_B10F:
      return 4;
    case GL_RGB9_E5:
      return 4;
    case GL_R8I:
      return 1;
    case GL_R8UI:
      return 1;
    case GL_R16I:
      return 2;
    case GL_R16UI:
      return 2;
    case GL_R32I:
      return 4;
    case GL_R32UI:
      return 4;
    case GL_RG8I:
      return 2;
    case GL_RG8UI:
      return 2;
    case GL_RG16I:
      return 4;
    case GL_RG16UI:
      return 4;
    case GL_RG32I:
      return 8;
    case GL_RG32UI:
      return 8;
    case GL_RGB8I:
      return 3;
    case GL_RGB8UI:
      return 3;
    case GL_RGB16I:
      return 6;
    case GL_RGB16UI:
      return 6;
    case GL_RGB32I:
      return 12;
    case GL_RGB32UI:
      return 12;
    case GL_RGBA8I:
      return 4;
    case GL_RGBA8UI:
      return 4;
    case GL_RGBA16I:
      return 8;
    case GL_RGBA16UI:
      return 8;
    case GL_RGBA32I:
      return 16;
    case GL_RGBA32UI:
      return 16;
    case GL_DEPTH_COMPONENT24:
      return 3;
    case GL_DEPTH_COMPONENT32:
      return 4;
    case GL_DEPTH_COMPONENT32F:
      return 4;
    default:
      return 1;
  }
}

GLuint64 CalcTexBytes2D(GLenum format, GLsizei level, GLuint64 size) {
  size = (level > 1) ? MipMap(k2D, size) : size;
  size = SizeOfFormat(format) * size;
  return size;
}

GLuint64 CalcTexBytes3D(GLenum format, GLsizei level, GLuint64 size) {
  size = (level > 1) ? MipMap(k3D, size) : size;
  size = SizeOfFormat(format) * size;
  return size;
}

GLuint64 MemoryUsed(Type type, GLuint64 read) {
  GLuint64 total = 0;
  for (const auto [obj, bytes] : gMaps[type]) {
    total += bytes;
  }
  return total / read;
}

}  // namespace mem