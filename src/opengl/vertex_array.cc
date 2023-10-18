#include "vertex_array.h"

// deps
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

namespace gl {

VertexArray::VertexArray() { glCreateVertexArrays(1, &vao_); }

VertexArray::~VertexArray() { glDeleteVertexArrays(1, &vao_); }

VertexArray::VertexArray(VertexArray&& o) noexcept
    : attrs_(std::move(o.attrs_)),
      buffers_(std::move(o.buffers_)),
      vao_(std::exchange(o.vao_, 0)),
      ebo_(std::exchange(o.ebo_, 0)) {}

VertexArray& VertexArray::operator=(VertexArray&& o) noexcept {
  if (&o == this) return *this;
  if (vao_) glDeleteVertexArrays(1, &vao_);

  attrs_ = std::move(o.attrs_);
  buffers_ = std::move(o.buffers_);
  vao_ = std::exchange(o.vao_, 0);
  ebo_ = std::exchange(o.ebo_, 0);

  return *this;
}

void VertexArray::EnableVertexAttrib(const VertexAttribute* attr,
                                     GLuint binding) {
  glEnableVertexArrayAttrib(vao_, attr->index);
  switch (attr->type) {
    case GL_FLOAT:
      glVertexArrayAttribFormat(vao_, attr->index, attr->num_of_comp,
                                attr->type, GL_FALSE, 0);
      break;
    case GL_HALF_FLOAT:
      glVertexArrayAttribFormat(vao_, attr->index, attr->num_of_comp,
                                attr->type, GL_FALSE, 0);
      break;
    case GL_DOUBLE:
      glVertexArrayAttribLFormat(vao_, attr->index, attr->num_of_comp,
                                 attr->type, 0);
      break;
    default:
      glVertexArrayAttribIFormat(vao_, attr->index, attr->num_of_comp,
                                 attr->type, 0);
      break;
  }
  glVertexArrayAttribBinding(vao_, attr->index, binding);
  if (attr->divisor) {
    glVertexArrayBindingDivisor(vao_, binding, attr->divisor);
  }
}

void VertexArray::EnableMatrixAttrib(const VertexAttribute* attr,
                                     GLuint binding) {
  for (GLuint row = 0; row < attr->num_of_comp; ++row) {
    glEnableVertexArrayAttrib(vao_, attr->index + row);
    GLuint relative_offset = row * (attr->stride / attr->num_of_comp);
    glVertexArrayAttribFormat(vao_, attr->index + row, attr->num_of_comp,
                              attr->type, GL_FALSE, relative_offset);
    glVertexArrayAttribBinding(vao_, attr->index + row, binding);
  }
  if (attr->divisor) {
    glVertexArrayBindingDivisor(vao_, binding, attr->divisor);
  }
}

void VertexArray::SetAttributes(const Attributes& attrs) {
  attrs_ = attrs;
  CheckAttributes(attrs_);

  buffers_.clear();
  buffers_.resize(attrs_.size(), 0);

  GLuint binding = 0;
  for (const auto* attr : attrs_) {
    if (attr->stride <= sizeof(glm::vec4)) {
      EnableVertexAttrib(attr, binding);
    } else {
      EnableMatrixAttrib(attr, binding);
    }
    ++binding;
  }
}

void VertexArray::SetVertexBuffers(const VertexBuffers& vertex_buffers) {
  const auto& attrs = vertex_buffers.GetAttrs();
  const auto& buffers = vertex_buffers.GetBuffers();

  for (size_t i = 0; i < attrs_.size(); ++i) {
    auto it = std::find(attrs.begin(), attrs.end(), attrs_[i]);
    if (it != attrs.end()) {
      auto vbo_index = std::distance(attrs.begin(), it);
      buffers_[i] = buffers[vbo_index];
    }
  }

  ebo_ = vertex_buffers.GetEbo();
}

void VertexArray::SetBuffer(const VertexAttribute* attr, GLuint buffer) {
  auto it = std::find(attrs_.begin(), attrs_.end(), attr);
  if (it != attrs_.end()) {
    auto binding = std::distance(attrs_.begin(), it);
    buffers_[binding] = buffer;
  } else {
    spdlog::error("{}: Incorrect attribute", __FUNCTION__);
  }
}

MiVector<GLsizei> VertexArray::GetStrides() const {
  MiVector<GLsizei> strides(attrs_.size(), 0);
  for (size_t i = 0; i < attrs_.size(); ++i) {
    strides[i] = attrs_[i]->stride;
  }
  return strides;
}

void VertexArray::AttachBuffers() const {
  MiVector<GLintptr> offsets(attrs_.size(), 0);
  MiVector<GLsizei> strides = GetStrides();
  glVertexArrayVertexBuffers(vao_, 0, static_cast<GLsizei>(buffers_.size()),
                             buffers_.data(), offsets.data(), strides.data());
  if (ebo_) glVertexArrayElementBuffer(vao_, ebo_);
}

void VertexArray::PrintVaoInfo() const {
  MiVector<const char*> names;
  for (const auto* attr : attrs_) {
    names.push_back(attr->name);
  }
  spdlog::info("Create VAO [{}] with attributes [{}]", vao_,
               fmt::join(names, " "));
  spdlog::info("buffers: {}, EBO: {}", buffers_, ebo_);
}

void VertexArray::Bind() const { glBindVertexArray(vao_); }

const MiVector<GLuint>& VertexArray::GetBuffers() const { return buffers_; }

GLuint VertexArray::GetEbo() const { return ebo_; }

}  // namespace gl