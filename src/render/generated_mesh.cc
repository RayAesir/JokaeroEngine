#include "generated_mesh.h"

// deps
#include <spdlog/spdlog.h>

GeneratedMesh::GeneratedMesh()
    : mesh_internal_({
          &kPositionAttr  //
      }),
      mesh_static_({
          &kPositionAttr,  //
          &kNormalAttr,    //
          &kTexCoordsAttr  //
      }) {
  mesh_internal_.SetStorage(kInternalSize.vertex_count,
                            kInternalSize.indice_count);
  mesh_static_.SetStorage(kStaticSize.vertex_count, kStaticSize.indice_count);

  pos_.SetAttributes({&kPositionAttr});
  pos_.SetVertexBuffers(mesh_internal_);
  pos_.AttachBuffers();
  pos_norm_tex_.SetAttributes({
      &kPositionAttr,  //
      &kNormalAttr,    //
      &kTexCoordsAttr  //
  });
  pos_norm_tex_.SetVertexBuffers(mesh_static_);
  pos_norm_tex_.AttachBuffers();

  GenSkybox(global::kFullRadius);
  GenPointLight(global::kFullRadius, global::kSlices);
  GenDebugBox(global::kFullRadius);

  GenCube(global::kHalfRadius, 1.0f);
  GenSphere(global::kHalfRadius, global::kSlices);
  GenCone(global::kHeight, global::kHalfRadius, global::kSlices,
          global::kStacks);
  GenCylinder(global::kHeight, global::kHalfRadius, global::kSlices);
  GenPlane(global::kHeight, global::kHeight, global::kPlaneSlicesStacks,
           global::kPlaneSlicesStacks);

  pos_.PrintVaoInfo();
  pos_norm_tex_.PrintVaoInfo();
}

void GeneratedMesh::CalcTangents(VertexDataStatic& data) {
  data.tangents.resize(data.positions.size());
  std::fill(data.tangents.begin(), data.tangents.end(), glm::vec3(0.0f));

  for (size_t i = 0; i < data.indices.size(); i += 3) {
    auto i0 = data.indices[i];
    auto i1 = data.indices[i + 1];
    auto i2 = data.indices[i + 2];

    auto edge1 = data.positions[i1] - data.positions[i0];
    auto edge2 = data.positions[i2] - data.positions[i0];

    auto delta_uv1 = data.tex_coords[i1] - data.tex_coords[i0];
    auto delta_uv2 = data.tex_coords[i2] - data.tex_coords[i0];

    float dividend = (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
    float f = ((dividend == 0.0f) ? 0.0f : 1.0f) / dividend;

    glm::vec3 tangent(0.0f);
    tangent.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
    tangent.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
    tangent.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

    data.tangents[i0] += tangent;
    data.tangents[i1] += tangent;
    data.tangents[i2] += tangent;
  }

  for (size_t i = 0; i < data.positions.size(); ++i) {
    data.tangents[i] = glm::normalize(data.tangents[i]);
  }
}

void GeneratedMesh::UploadMeshInternal(VertexDataInternal& data,
                                       GenMeshInternal::Enum type) {
  MiVector<void*> upload = {static_cast<void*>(data.positions.data())};
  GLuint vertex_count = static_cast<GLuint>(data.positions.size());
  GLuint indice_count = static_cast<GLuint>(data.indices.size());
  addr_internal_[type] = mesh_internal_.UploadVerticesIndicesMt(
      upload, vertex_count, static_cast<void*>(data.indices.data()),
      indice_count);
}

void GeneratedMesh::UploadMeshStatic(VertexDataStatic& data,
                                     GenMeshStatic::Enum type) {
  MiVector<void*> upload = {
      static_cast<void*>(data.positions.data()),
      static_cast<void*>(data.normals.data()),
      static_cast<void*>(data.tex_coords.data()),
  };
  GLuint vertex_count = static_cast<GLuint>(data.positions.size());
  GLuint indice_count = static_cast<GLuint>(data.indices.size());
  addr_static_[type] = mesh_static_.UploadVerticesIndicesMt(
      upload, vertex_count, static_cast<void*>(data.indices.data()),
      indice_count);
}

void GeneratedMesh::GenDebugBox(float radius) {
  VertexDataInternal data{CalcDebugBox()};
  float r2 = radius;

  data.positions = {
      glm::vec3(+r2, +r2, -r2), glm::vec3(+r2, -r2, -r2),
      glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, -r2, +r2),

      glm::vec3(-r2, +r2, -r2), glm::vec3(-r2, -r2, -r2),
      glm::vec3(-r2, +r2, +r2), glm::vec3(-r2, -r2, +r2),
  };

  data.indices = {
      5, 7, 1, 5, 0, 1, 7, 6, 2, 3, 4, 5,  //
      2, 6, 0, 2, 7, 3, 6, 4, 4, 0, 3, 1,  //
  };

  UploadMeshInternal(data, GenMeshInternal::kDebugBox);
}

void GeneratedMesh::GenSkybox(float radius) {
  VertexDataInternal data{CalcCube()};
  float r2 = radius;

  data.positions = {// Bottom Y-
                    glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, -r2, +r2),
                    glm::vec3(+r2, -r2, +r2), glm::vec3(+r2, -r2, -r2),
                    // Top Y+
                    glm::vec3(-r2, +r2, -r2), glm::vec3(-r2, +r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, +r2, -r2),
                    // Back Z-
                    glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, +r2, -r2),
                    glm::vec3(+r2, +r2, -r2), glm::vec3(+r2, -r2, -r2),
                    // Front Z+
                    glm::vec3(-r2, -r2, +r2), glm::vec3(-r2, +r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, -r2, +r2),
                    // Right X-
                    glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, -r2, +r2),
                    glm::vec3(-r2, +r2, +r2), glm::vec3(-r2, +r2, -r2),
                    // Left X+
                    glm::vec3(+r2, -r2, -r2), glm::vec3(+r2, -r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, +r2, -r2)};

  data.indices = {0,  2,  1,  0,  3,  2,  4,  5,  6,  4,  6,  7,
                  8,  9,  10, 8,  10, 11, 12, 15, 14, 12, 14, 13,
                  16, 17, 18, 16, 18, 19, 20, 23, 22, 20, 22, 21};

  UploadMeshInternal(data, GenMeshInternal::kSkybox);
}

void GeneratedMesh::GenPointLight(float radius, unsigned int slices) {
  VertexDataInternal data{CalcSphere(slices)};
  float delta_phi = glm::two_pi<float>() / static_cast<float>(slices);
  unsigned int parallels = static_cast<unsigned int>(slices * 0.5f);

  for (unsigned int i = 0; i <= parallels; ++i) {
    for (unsigned int j = 0; j <= slices; ++j) {
      data.positions.emplace_back(
          radius * glm::sin(delta_phi * i) * glm::sin(delta_phi * j),
          radius * glm::cos(delta_phi * i),
          radius * glm::sin(delta_phi * i) * glm::cos(delta_phi * j));
    }
  }

  for (unsigned int i = 0; i < parallels; ++i) {
    for (unsigned int j = 0; j < slices; ++j) {
      data.indices.push_back(i * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + (j + 1));

      data.indices.push_back(i * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + (j + 1));
      data.indices.push_back(i * (slices + 1) + (j + 1));
    }
  }

  UploadMeshInternal(data, GenMeshInternal::kPointLight);
}

void GeneratedMesh::GenCube(float radius, float tex_coords_scale) {
  VertexDataStatic data{CalcCube()};
  float r2 = radius;

  data.positions = {glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, -r2, +r2),
                    glm::vec3(+r2, -r2, +r2), glm::vec3(+r2, -r2, -r2),

                    glm::vec3(-r2, +r2, -r2), glm::vec3(-r2, +r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, +r2, -r2),

                    glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, +r2, -r2),
                    glm::vec3(+r2, +r2, -r2), glm::vec3(+r2, -r2, -r2),

                    glm::vec3(-r2, -r2, +r2), glm::vec3(-r2, +r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, -r2, +r2),

                    glm::vec3(-r2, -r2, -r2), glm::vec3(-r2, -r2, +r2),
                    glm::vec3(-r2, +r2, +r2), glm::vec3(-r2, +r2, -r2),

                    glm::vec3(+r2, -r2, -r2), glm::vec3(+r2, -r2, +r2),
                    glm::vec3(+r2, +r2, +r2), glm::vec3(+r2, +r2, -r2)};

  data.normals = {glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),

                  glm::vec3(0.0f, +1.0f, 0.0f), glm::vec3(0.0f, +1.0f, 0.0f),
                  glm::vec3(0.0f, +1.0f, 0.0f), glm::vec3(0.0f, +1.0f, 0.0f),

                  glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f),

                  glm::vec3(0.0f, 0.0f, +1.0f), glm::vec3(0.0f, 0.0f, +1.0f),
                  glm::vec3(0.0f, 0.0f, +1.0f), glm::vec3(0.0f, 0.0f, +1.0f),

                  glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),

                  glm::vec3(+1.0f, 0.0f, 0.0f), glm::vec3(+1.0f, 0.0f, 0.0f),
                  glm::vec3(+1.0f, 0.0f, 0.0f), glm::vec3(+1.0f, 0.0f, 0.0f)};

  data.tex_coords = {glm::vec2(0.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,

                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale,

                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 0.0f) * tex_coords_scale,

                     glm::vec2(0.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,

                     glm::vec2(0.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,

                     glm::vec2(1.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 0.0f) * tex_coords_scale,
                     glm::vec2(0.0f, 1.0f) * tex_coords_scale,
                     glm::vec2(1.0f, 1.0f) * tex_coords_scale};

  data.indices = {0,  2,  1,  0,  3,  2,  4,  5,  6,  4,  6,  7,
                  8,  9,  10, 8,  10, 11, 12, 15, 14, 12, 14, 13,
                  16, 17, 18, 16, 18, 19, 20, 23, 22, 20, 22, 21};

  UploadMeshStatic(data, GenMeshStatic::kCube);
}

void GeneratedMesh::GenSphere(float radius, unsigned int slices) {
  VertexDataStatic data{CalcSphere(slices)};
  float delta_phi = glm::two_pi<float>() / static_cast<float>(slices);
  unsigned int parallels = static_cast<unsigned int>(slices * 0.5f);

  for (unsigned int i = 0; i <= parallels; ++i) {
    for (unsigned int j = 0; j <= slices; ++j) {
      data.positions.emplace_back(
          radius * glm::sin(delta_phi * i) * glm::sin(delta_phi * j),
          radius * glm::cos(delta_phi * i),
          radius * glm::sin(delta_phi * i) * glm::cos(delta_phi * j));

      data.normals.emplace_back(
          radius * glm::sin(delta_phi * i) * glm::sin(delta_phi * j) / radius,
          radius * glm::cos(delta_phi * i) / radius,
          radius * glm::sin(delta_phi * i) * glm::cos(delta_phi * j) / radius);

      data.tex_coords.emplace_back(j / static_cast<float>(slices),
                                   1.0f - i / static_cast<float>(parallels));
    }
  }

  for (unsigned int i = 0; i < parallels; ++i) {
    for (unsigned int j = 0; j < slices; ++j) {
      data.indices.push_back(i * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + (j + 1));

      data.indices.push_back(i * (slices + 1) + j);
      data.indices.push_back((i + 1) * (slices + 1) + (j + 1));
      data.indices.push_back(i * (slices + 1) + (j + 1));
    }
  }

  UploadMeshStatic(data, GenMeshStatic::kSphere);
}

void GeneratedMesh::GenCone(float height, float radius, unsigned int slices,
                            unsigned int stacks) {
  VertexDataStatic data{CalcCone(slices, stacks)};
  float theta_inc = glm::two_pi<float>() / float(slices);
  float theta = 0.0f;

  // center bottom
  glm::vec3 p{0.0f, height, 0.0f};
  data.positions.emplace_back(-p);
  data.normals.emplace_back(0.0f, -1.0f, 0.0f);
  data.tex_coords.emplace_back(0.5f, 0.5f);

  // bottom
  for (unsigned int side_count = 0; side_count <= slices;
       ++side_count, theta += theta_inc) {
    data.positions.emplace_back(glm::cos(theta) * radius, -height,
                                -glm::sin(theta) * radius);
    data.normals.emplace_back(0.0f, -1.0f, 0.0f);
    data.tex_coords.emplace_back(glm::cos(theta) * 0.5f + 0.5f,
                                 glm::sin(theta) * 0.5f + 0.5f);
  }

  // sides
  float l = glm::sqrt(height * height + radius * radius);

  for (unsigned int stack_count = 0; stack_count <= stacks; ++stack_count) {
    float level = stack_count / static_cast<float>(stacks);

    for (unsigned int slice_count = 0; slice_count <= slices;
         ++slice_count, theta += theta_inc) {
      data.positions.emplace_back(glm::cos(theta) * radius * (1.0f - level),
                                  -height + height * level,
                                  -glm::sin(theta) * radius * (1.0f - level));
      data.normals.emplace_back(glm::cos(theta) * height / l, radius / l,
                                -glm::sin(theta) * height / l);
      data.tex_coords.emplace_back(slice_count / float(slices), level);
    }
  }

  unsigned int center_idx = 0;
  unsigned int idx = 1;

  // indices bottom
  for (unsigned int slice_count = 0; slice_count < slices; ++slice_count) {
    data.indices.push_back(center_idx);
    data.indices.push_back(idx + 1);
    data.indices.push_back(idx);

    ++idx;
  }
  ++idx;

  // indices sides
  for (unsigned int stack_count = 0; stack_count < stacks; ++stack_count) {
    for (unsigned int slice_count = 0; slice_count < slices; ++slice_count) {
      data.indices.push_back(idx);
      data.indices.push_back(idx + 1);
      data.indices.push_back(idx + slices + 1);

      data.indices.push_back(idx + 1);
      data.indices.push_back(idx + slices + 2);
      data.indices.push_back(idx + slices + 1);

      ++idx;
    }
    ++idx;
  }

  UploadMeshStatic(data, GenMeshStatic::kCone);
}

void GeneratedMesh::GenCylinder(float height, float radius,
                                unsigned int slices) {
  VertexDataStatic data{CalcCylinder(slices)};
  float half_height = height * 0.5f;
  glm::vec3 p1{0.0f, half_height, 0.0f};
  glm::vec3 p2 = -p1;

  float theta_inc = glm::two_pi<float>() / float(slices);
  float theta = 0.0f;
  float sign = -1.0f;

  // center bottom
  data.positions.emplace_back(p2);
  data.normals.emplace_back(0.0f, -1.0f, 0.0f);
  data.tex_coords.emplace_back(0.5f, 0.5f);

  // bottom
  for (unsigned int side_count = 0; side_count <= slices;
       ++side_count, theta += theta_inc) {
    data.positions.emplace_back(glm::cos(theta) * radius, -half_height,
                                -glm::sin(theta) * radius);
    data.normals.emplace_back(0.0f, -1.0f, 0.0f);
    data.tex_coords.emplace_back(glm::cos(theta) * 0.5f + 0.5f,
                                 glm::sin(theta) * 0.5f + 0.5f);
  }

  // center top
  data.positions.emplace_back(p1);
  data.normals.emplace_back(0.0f, 1.0f, 0.0f);
  data.tex_coords.emplace_back(0.5f, 0.5f);

  // top
  for (unsigned int side_count = 0; side_count <= slices;
       ++side_count, theta += theta_inc) {
    data.positions.emplace_back(glm::cos(theta) * radius, half_height,
                                -glm::sin(theta) * radius);
    data.normals.emplace_back(0.0f, 1.0f, 0.0f);
    data.tex_coords.emplace_back(glm::cos(theta) * 0.5f + 0.5f,
                                 glm::sin(theta) * 0.5f + 0.5f);
  }

  // sides
  for (unsigned int side_count = 0; side_count <= slices;
       ++side_count, theta += theta_inc) {
    sign = -1.0f;

    for (int i = 0; i < 2; ++i) {
      data.positions.emplace_back(glm::cos(theta) * radius, half_height * sign,
                                  -glm::sin(theta) * radius);
      data.normals.emplace_back(glm::cos(theta), 0.0f, -glm::sin(theta));
      data.tex_coords.emplace_back(side_count / (float)slices,
                                   (sign + 1.0f) * 0.5f);

      sign = 1.0f;
    }
  }

  unsigned int center_idx = 0;
  unsigned int idx = 1;

  // indices bottom
  for (unsigned int side_count = 0; side_count < slices; ++side_count) {
    data.indices.push_back(center_idx);
    data.indices.push_back(idx + 1);
    data.indices.push_back(idx);

    ++idx;
  }
  ++idx;

  // indices top
  center_idx = idx;
  ++idx;

  for (unsigned int side_count = 0; side_count < slices; ++side_count) {
    data.indices.push_back(center_idx);
    data.indices.push_back(idx);
    data.indices.push_back(idx + 1);

    ++idx;
  }
  ++idx;

  // indices sides
  for (unsigned int side_count = 0; side_count < slices; ++side_count) {
    data.indices.push_back(idx);
    data.indices.push_back(idx + 2);
    data.indices.push_back(idx + 1);

    data.indices.push_back(idx + 2);
    data.indices.push_back(idx + 3);
    data.indices.push_back(idx + 1);

    idx += 2;
  }

  UploadMeshStatic(data, GenMeshStatic::kCylinder);
}

void GeneratedMesh::GenPlane(float width, float height, unsigned int slices,
                             unsigned int stacks) {
  VertexDataStatic data{CalcPlane(slices, stacks)};
  float width_inc = width / float(slices);
  float height_inc = height / float(stacks);

  float w = -width * 0.5f;
  float h = -height * 0.5f;

  for (unsigned int j = 0; j <= stacks; ++j, h += height_inc) {
    for (unsigned int i = 0; i <= slices; ++i, w += width_inc) {
      data.positions.emplace_back(w, 0.0f, h);
      data.normals.emplace_back(0.0f, 1.0f, 0.0f);
      data.tex_coords.emplace_back(i, j);
    }
    w = -width * 0.5f;
  }

  unsigned int idx = 0;

  for (unsigned int j = 0; j < stacks; ++j) {
    for (unsigned int i = 0; i < slices; ++i) {
      data.indices.push_back(idx);
      data.indices.push_back(idx + slices + 1);
      data.indices.push_back(idx + 1);

      data.indices.push_back(idx + 1);
      data.indices.push_back(idx + slices + 1);
      data.indices.push_back(idx + slices + 2);

      ++idx;
    }

    ++idx;
  }

  UploadMeshStatic(data, GenMeshStatic::kPlane);
}

void GeneratedMesh::DrawInternalMesh(GenMeshInternal::Enum type,
                                     GLenum mode) const {
  pos_.Bind();
  addr_internal_[type].Draw(mode);
}

void GeneratedMesh::DrawInternalMeshInstanced(GenMeshInternal::Enum type,
                                              GLenum mode, GLuint count) const {
  pos_.Bind();
  addr_internal_[type].DrawInstanced(mode, count);
}

void GeneratedMesh::DrawStaticMesh(GenMeshStatic::Enum type,
                                   GLenum mode) const {
  pos_norm_tex_.Bind();
  addr_static_[type].Draw(mode);
}

const gl::VertexAddr& GeneratedMesh::GetVertexAddr(
    GenMeshInternal::Enum type) const {
  return addr_internal_[type];
}

const gl::VertexAddr& GeneratedMesh::GetVertexAddr(
    GenMeshStatic::Enum type) const {
  return addr_static_[type];
}
