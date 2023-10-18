#pragma once

// deps
#include <spdlog/spdlog.h>
// global
#include <string>
// local
#include "global.h"
#include "mem_info.h"
#include "mi_types.h"

// layout(std430, binding = x)
struct ShaderStorageBinding {
  enum Enum : GLuint {
    // draw batch
    kIndirectCmdStorage,
    kIndirectCmdDraw,
    kAttributeInstanceData,
    kAttributeInstanceMatrixMvp,
    // frustum/occlusion culling
    kAtomicCounters,
    kInstances,
    kVisibility,
    kMatrices,
    kStaticBoxes,
    kSkinnedBoxes,
    kAnimations,
    kMaterials,
    // lights
    kPointLights,
    kFrustumPointLights,
    kFrustumPointShadows,
    kFrustumClusters,
    kClusterLights,
    // particles
    kFxPresets,
    kFxInstances,
    kFxBoxes,
    // misc
    kPickEntity,
    kCrosshair,
    kDebugReadback,
    kShowCameras,
    kTotal,
  };
};

namespace gl {

struct BufferAddr {
  GLintptr element_offset;
  GLint index_offset;
};

// Immutable Storage represented by struct
template <typename S>
class Buffer {
 public:
  Buffer(const std::string& name);
  ~Buffer();
  // OpenGL resources (move-only)
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& o) noexcept;
  Buffer& operator=(Buffer&& o) noexcept;

 public:
  operator GLuint() const;
  void ReBindBuffer() const;
  void BindBuffer(GLenum target, GLuint binding);
  void SetStorage(GLbitfield create_flags = GL_DYNAMIC_STORAGE_BIT);
  void ClearData() const;
  void ClearSubData(GLintptr offset, GLsizeiptr size) const;

 protected:
  std::string name_;
  GLuint vbo_{0};
  GLenum target_{0};
  GLuint binding_{0};
  GLbitfield create_flags_{0};
  GLsizeiptr total_size_{0};
};

template <typename S>
class CountedBuffer : public Buffer<S> {
 public:
  using Buffer<S>::Buffer;
  using Buffer<S>::name_;
  using Buffer<S>::vbo_;
  using Buffer<S>::total_size_;

  template <typename E>
  BufferAddr AppendArray(const E* data, GLsizeiptr count);
  template <typename E>
  BufferAddr AppendVector(const MiVector<E>& vec);
  template <typename E>
  void UploadIndex(const E& value, GLuint index);

 protected:
  GLsizeiptr current_size_{0};
  GLintptr element_count_{0};
  GLint upload_count_{0};
};

template <typename S>
class StreamBuffer : public Buffer<S> {
 public:
  using Buffer<S>::Buffer;
  using Buffer<S>::name_;
  using Buffer<S>::vbo_;

  template <typename E, typename R>
  void UploadArray(R S::*member, const E* data, GLsizeiptr count) const;
  template <typename E, typename R>
  void UploadVector(R S::*member, const MiVector<E>& vec) const;
};

template <typename S>
class MappedBuffer : public Buffer<S> {
 public:
  using Buffer<S>::vbo_;
  using Buffer<S>::total_size_;
  MappedBuffer(const std::string& name);
  // OpenGL resources (move-only)
  MappedBuffer(const MappedBuffer&) = delete;
  MappedBuffer& operator=(const MappedBuffer&) = delete;
  MappedBuffer(MappedBuffer&&) noexcept = default;
  MappedBuffer& operator=(MappedBuffer&&) noexcept = default;

 public:
  operator S*() const;
  S* operator->() const;
  void MapRange(GLbitfield map_flags);

 protected:
  S* ptr_{nullptr};
  GLbitfield map_flags_{0};
};

}  // namespace gl

#include "buffer_storage.inl"