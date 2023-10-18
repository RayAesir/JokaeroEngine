#include "vertex_buffers.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "mem_info.h"

namespace gl {

void VertexAddr::Draw(GLenum mode) const {
  glDrawElementsBaseVertex(mode, indice_count, GL_UNSIGNED_INT, first_index,
                           vertex_offset);
}

void VertexAddr::DrawInstanced(GLenum mode, GLuint count) const {
  glDrawElementsInstancedBaseVertex(mode,             //
                                    indice_count,     //
                                    GL_UNSIGNED_INT,  //
                                    first_index,      //
                                    count,            //
                                    vertex_offset     //
  );
}

void VertexAddr::DrawInstancedBaseInstance(GLenum mode, GLuint count,
                                           GLuint base) const {
  glDrawElementsInstancedBaseVertexBaseInstance(GL_LINES,         //
                                                indice_count,     //
                                                GL_UNSIGNED_INT,  //
                                                first_index,      //
                                                count,            //
                                                vertex_offset,    //
                                                base);
}

void CheckAttributes(Attributes& attrs) {
  // sort the input attributes by location index
  auto by_index = [](const VertexAttribute* a, const VertexAttribute* b) {
    return a->index < b->index;
  };
  std::sort(attrs.begin(), attrs.end(), by_index);
  // remove duplicates
  auto by_ptr = [](const VertexAttribute* a, const VertexAttribute* b) {
    return a == b;
  };
  size_t prev_size = attrs.size();
  attrs.erase(std::unique(attrs.begin(), attrs.end(), by_ptr), attrs.end());
  if (prev_size != attrs.size()) {
    spdlog::warn("{}: Duplicated vertex attributes", __FUNCTION__);
  }
}

VertexBuffers::VertexBuffers(const Attributes& attrs) : attrs_(attrs) {
  CheckAttributes(attrs_);

  buffers_.resize(attrs_.size(), 0);
  strides_.reserve(attrs_.size());
  for (const auto* attr : attrs_) {
    strides_.push_back(attr->stride);
  }

  glCreateBuffers(static_cast<GLsizei>(buffers_.size()), buffers_.data());
  glCreateBuffers(1, &ebo_);
}

VertexBuffers::~VertexBuffers() {
  glDeleteBuffers(static_cast<GLsizei>(buffers_.size()), buffers_.data());
  glDeleteBuffers(1, &ebo_);
  mem::Erase(mem::kVertexBuffer, this);
}

VertexBuffers::VertexBuffers(VertexBuffers&& o) noexcept
    : attrs_(std::move(o.attrs_)),
      buffers_(
          std::exchange(o.buffers_, MiVector<GLuint>(o.buffers_.size(), 0))),
      strides_(std::move(o.strides_)),
      ebo_(std::exchange(o.ebo_, 0)),

      vertex_current_count_(std::exchange(o.vertex_current_count_, 0)),
      indice_current_count_(std::exchange(o.indice_current_count_, 0)),
      vertex_total_count_(std::exchange(o.vertex_total_count_, 0)),
      indice_total_count_(std::exchange(o.indice_total_count_, 0)),

      sync_(std::move(o.sync_)) {
  mem::Move(mem::kVertexBuffer, &o, this);
}

VertexBuffers& VertexBuffers::operator=(VertexBuffers&& o) noexcept {
  if (&o == this) return *this;
  if (buffers_.size()) VertexBuffers::~VertexBuffers();

  attrs_ = std::move(o.attrs_);
  buffers_ = std::exchange(o.buffers_, MiVector<GLuint>(o.buffers_.size(), 0));
  strides_ = std::move(o.strides_);
  ebo_ = std::exchange(o.ebo_, 0);

  vertex_current_count_ = std::exchange(o.vertex_current_count_, 0);
  indice_current_count_ = std::exchange(o.indice_current_count_, 0);
  vertex_total_count_ = std::exchange(o.vertex_total_count_, 0);
  indice_total_count_ = std::exchange(o.indice_total_count_, 0);

  sync_ = std::move(o.sync_);
  mem::Move(mem::kVertexBuffer, &o, this);

  return *this;
}

void VertexBuffers::SetStorage(GLuint vertex_count, GLuint indice_count) {
  vertex_total_count_ = vertex_count;
  indice_total_count_ = indice_count;

  GLuint64 mem_size = 0;
  for (size_t i = 0; i < buffers_.size(); ++i) {
    GLsizeiptr size = strides_[i] * vertex_total_count_;
    glNamedBufferStorage(buffers_[i], size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    mem_size += size;
  }

  GLsizeiptr size = sizeof(GLuint) * indice_total_count_;
  glNamedBufferStorage(ebo_, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
  mem_size += size;

  mem::Add(mem::kVertexBuffer, this, mem_size);
}

const Attributes& VertexBuffers::GetAttrs() const { return attrs_; }

const MiVector<GLuint>& VertexBuffers::GetBuffers() const { return buffers_; }

GLuint VertexBuffers::GetEbo() const { return ebo_; }

bool VertexBuffers::NotEnoughSpaceMt(GLuint vertex_count,
                                     GLuint indice_count) const {
  std::scoped_lock lock(mutex_);
  if (vertex_total_count_ < vertex_current_count_ + vertex_count) {
    spdlog::error("{}: Out of memory, vertices", __FUNCTION__);
    return true;
  }
  if (indice_total_count_ < indice_current_count_ + indice_count) {
    spdlog::error("{}: Out of memory, indices", __FUNCTION__);
    return true;
  }
  return false;
}

VertexAddr VertexBuffers::UploadVerticesIndicesMt(
    const MiVector<void*>& vertices,  //
    GLuint vertex_count,              //
    void* indices,                    //
    GLuint indice_count               //
) {
  std::scoped_lock lock(mutex_);

  VertexAddr addr{
      .vertex_count = vertex_count,            //
      .vertex_offset = vertex_current_count_,  //
      .indice_count = indice_count,            //
      .indice_offset = indice_current_count_,  //
      .first_index = reinterpret_cast<GLvoid*>(
          static_cast<GLuint64>(sizeof(GLuint) * indice_current_count_))  //
  };

  sync_.BeginMt();

  for (size_t i = 0; i < buffers_.size(); ++i) {
    GLintptr offset = strides_[i] * vertex_current_count_;
    GLsizeiptr size = strides_[i] * vertex_count;
    glNamedBufferSubData(buffers_[i], offset, size, vertices[i]);
  }
  vertex_current_count_ += vertex_count;

  GLintptr offset = sizeof(GLuint) * indice_current_count_;
  GLsizeiptr size = sizeof(GLuint) * indice_count;
  glNamedBufferSubData(ebo_, offset, size, indices);
  indice_current_count_ += indice_count;

  sync_.EndMt();

  return addr;
}

}  // namespace gl