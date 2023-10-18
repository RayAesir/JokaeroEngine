#include "uniform_buffer.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "app/parameters.h"
#include "global.h"

namespace gl {

UniformBuffer::UniformBuffer() noexcept {
  glCreateBuffers(1, &ubo_);
  GLsizeiptr size = static_cast<GLsizeiptr>(app::gpu.max_ubo_block_size);
  glNamedBufferStorage(ubo_, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
  // reserve max size to avoid pointers invalidation
  raw_memory_.reserve(app::gpu.max_ubo_block_size);
}

UniformBuffer::~UniformBuffer() { glDeleteBuffers(1, &ubo_); }

UniformBuffer::UniformBuffer(UniformBuffer &&o) noexcept
    : ubo_(std::exchange(o.ubo_, 0)),
      offsets_(std::move(o.offsets_)),
      raw_memory_(std::move(o.raw_memory_)){};

UniformBuffer &UniformBuffer::operator=(UniformBuffer &&o) noexcept {
  if (&o == this) return *this;
  if (ubo_) glDeleteBuffers(1, &ubo_);

  ubo_ = std::exchange(o.ubo_, 0);
  offsets_ = std::move(o.offsets_);
  raw_memory_ = std::move(o.raw_memory_);
  return *this;
}

void *UniformBuffer::MapBufferImpl(GLsizeiptr size, GLuint binding) noexcept {
  // buffer's size has to be divisible by offset alignment (e.g. 256 bytes)
  // 64 -> 256, 384 -> 512, etc.
  GLsizeiptr aligned_size = CeilInt(size, app::gpu.ubo_offset_alignment) *
                            app::gpu.ubo_offset_alignment;
  // already aligned
  GLintptr current_size = static_cast<GLintptr>(raw_memory_.size());
  // check overflow
  if (aligned_size + current_size > app::gpu.max_ubo_block_size) {
    spdlog::error("{}: Max UBO size is '{}'", __FUNCTION__,
                  app::gpu.max_ubo_block_size);
    return nullptr;
  }
  offsets_.push_back(current_size);
  raw_memory_.insert(raw_memory_.end(), aligned_size, 0);
  // without glBindBuffersRange, binding order unknown
  glBindBufferRange(GL_UNIFORM_BUFFER, binding, ubo_, current_size,
                    aligned_size);

  return static_cast<void *>(&raw_memory_[current_size]);
}

void UniformBuffer::Upload() const {
  glNamedBufferSubData(ubo_, 0, static_cast<GLsizeiptr>(raw_memory_.size()),
                       raw_memory_.data());
}

void UniformBuffer::SubData(GLuint binding, GLsizeiptr size, void *data) const {
  glNamedBufferSubData(ubo_, offsets_[binding], size, data);
}

}  // namespace gl