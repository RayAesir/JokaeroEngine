#include "buffer_storage.h"

namespace gl {

template <typename S>
Buffer<S>::Buffer(const std::string& name)
    : name_(name), total_size_(sizeof(S)) {
  glCreateBuffers(1, &vbo_);
}

template <typename S>
Buffer<S>::~Buffer() {
  glDeleteBuffers(1, &vbo_);
  mem::Erase(mem::kBuffer, this);
}

template <typename S>
Buffer<S>::Buffer(Buffer&& o) noexcept
    : name_(std::move(o.name_)),
      vbo_(std::exchange(o.vbo_, 0)),
      target_(std::exchange(o.target_, 0)),
      binding_(std::exchange(o.binding_, 0)),
      create_flags_(std::exchange(o.create_flags_, 0)),
      total_size_(std::exchange(o.total_size_, 0)) {
  mem::Move(mem::kBuffer, &o, this);
}

template <typename S>
Buffer<S>& Buffer<S>::operator=(Buffer&& o) noexcept {
  if (&o == this) return *this;
  if (vbo_) glDeleteBuffers(1, &vbo_);

  name_ = std::move(o.name_);
  vbo_ = std::exchange(o.vbo_, 0);
  target_ = std::exchange(o.target_, 0);
  binding_ = std::exchange(o.binding_, 0);
  create_flags_ = std::exchange(o.create_flags_, 0);
  total_size_ = std::exchange(o.total_size_, 0);
  mem::Move(mem::kBuffer, &o, this);
  return *this;
}

template <typename S>
Buffer<S>::operator GLuint() const {
  return vbo_;
}

template <typename S>
void Buffer<S>::ReBindBuffer() const {
  if (target_) {
    glBindBufferBase(target_, binding_, vbo_);
  }
}

template <typename S>
void Buffer<S>::BindBuffer(GLenum target, GLuint binding) {
  target_ = target;
  binding_ = binding_;
  glBindBufferBase(target, binding, vbo_);
}

template <typename S>
void Buffer<S>::SetStorage(GLbitfield create_flags) {
  create_flags_ = create_flags;
  glNamedBufferStorage(vbo_, total_size_, nullptr, create_flags_);
  glClearNamedBufferData(vbo_, GL_R32UI, GL_RED, GL_UNSIGNED_INT,
                         &global::kZero4Bytes);
  mem::Add(mem::kBuffer, this, total_size_);
}

template <typename S>
void Buffer<S>::ClearData() const {
  glClearNamedBufferData(vbo_, GL_R32UI, GL_RED, GL_UNSIGNED_INT,
                         &global::kZero4Bytes);
}

template <typename S>
void Buffer<S>::ClearSubData(GLintptr offset, GLsizeiptr size) const {
  glClearNamedBufferSubData(vbo_, GL_R32UI, offset, size, GL_RED,
                            GL_UNSIGNED_INT, &global::kZero4Bytes);
}

template <typename S>
template <typename E>
BufferAddr CountedBuffer<S>::AppendArray(const E* data, GLsizeiptr count) {
  GLsizeiptr new_size = sizeof(E) * (element_count_ + count);
  BufferAddr addr{element_count_, upload_count_};
  if (new_size <= total_size_) {
    GLintptr offset_bytes = sizeof(E) * element_count_;
    GLsizeiptr size_bytes = sizeof(E) * count;
    glNamedBufferSubData(vbo_, offset_bytes, size_bytes, data);
    element_count_ += count;
    ++upload_count_;
    current_size_ += size_bytes;
  } else {
    spdlog::error("{}: Buffer '{}' overflow, size: '{}'", __FUNCTION__, name_,
                  total_size_);
  }
  return addr;
}

template <typename S>
template <typename E>
BufferAddr CountedBuffer<S>::AppendVector(const MiVector<E>& vec) {
  GLsizeiptr count = static_cast<GLsizeiptr>(vec.size());
  GLsizeiptr new_size = sizeof(E) * (element_count_ + count);
  BufferAddr addr{element_count_, upload_count_};
  if (new_size <= total_size_) {
    GLintptr offset_bytes = sizeof(E) * element_count_;
    GLsizeiptr size_bytes = sizeof(E) * count;
    glNamedBufferSubData(vbo_, offset_bytes, size_bytes, vec.data());
    element_count_ += count;
    ++upload_count_;
    current_size_ += size_bytes;
  } else {
    spdlog::error("{}: Buffer overflow '{}'", __FUNCTION__, name_);
  }
  return addr;
};

template <typename S>
template <typename E>
void CountedBuffer<S>::UploadIndex(const E& value, GLuint index) {
  GLintptr offset = sizeof(E) * static_cast<GLintptr>(index);
  GLsizeiptr size = sizeof(E);
  if (offset < total_size_) {
    glNamedBufferSubData(vbo_, offset, size, &value);
  } else {
    spdlog::error("{}: Buffer '{}', index is out of range '{}'", __FUNCTION__,
                  name_, index);
  }
}

template <typename S>
template <typename E, typename R>
void StreamBuffer<S>::UploadArray(R S::*member, const E* data,
                                  GLsizeiptr count) const {
  constexpr GLsizeiptr limit = sizeof((((S*)0)->*member));
  GLsizeiptr size = sizeof(E) * count;
  if (size <= limit) {
    GLintptr offset = reinterpret_cast<GLintptr>(
        &reinterpret_cast<char const volatile&>((((S*)0)->*member)));
    glNamedBufferSubData(vbo_, offset, size, data);
  } else {
    spdlog::error("{}: Buffer overflow '{}'", __FUNCTION__, name_);
  }
};

template <typename S>
template <typename E, typename R>
void StreamBuffer<S>::UploadVector(R S::*member, const MiVector<E>& vec) const {
  constexpr GLsizeiptr limit = sizeof((((S*)0)->*member));
  GLsizeiptr size = sizeof(E) * static_cast<GLsizeiptr>(vec.size());
  if (size <= limit) {
    GLintptr offset = reinterpret_cast<GLintptr>(
        &reinterpret_cast<char const volatile&>((((S*)0)->*member)));
    glNamedBufferSubData(vbo_, offset, size, vec.data());
  } else {
    spdlog::error("{}: Buffer overflow '{}'", __FUNCTION__, name_);
  }
};

template <typename S>
MappedBuffer<S>::MappedBuffer(const std::string& name) : Buffer<S>(name) {}

template <typename S>
MappedBuffer<S>::operator S*() const {
  return ptr_;
}

template <typename S>
S* MappedBuffer<S>::operator->() const {
  return ptr_;
}

template <typename S>
void MappedBuffer<S>::MapRange(GLbitfield map_flags) {
  map_flags_ = map_flags;
  /*
  GL_INVALID_OPERATION is generated if either the source or destination
  buffer object is mapped with glMapBufferRange or glMapBuffer, unless they
  were mapped with the GL_MAP_PERSISTENT bit set in the glMapBufferRange
  access flags.
  */

  /*
  GL_DYNAMIC_STORAGE_BIT
 The contents of the data store may be updated after creation through calls
 to glBufferSubData. If this bit is not set, the buffer content may not be
 directly updated by the client. The data argument may be used to specify
 the initial content of the buffer's data store regardless of the presence
 of the GL_DYNAMIC_STORAGE_BIT. Regardless of the presence of this bit,
 buffers may always be updated with server-side calls such as
 glCopyBufferSubData and glClearBufferSubData.
  */

  ptr_ =
      static_cast<S*>(glMapNamedBufferRange(vbo_, 0, total_size_, map_flags));
}

}  // namespace gl