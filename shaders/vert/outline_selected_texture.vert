#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 4) in vec2 aTexCoords;
layout(location = 6) in ivec4 aBonesIds;
layout(location = 7) in vec4 aWeights;

// out
out VertexData {
  vec2 tex_coords;
  vec3 view_normal;
}
vs_out;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_ANIMATIONS readonly
#include "shaders/ssbo/animations.glsl"

// uniform
layout(location = 0) uniform mat4 uModel;
layout(location = 2) uniform uint uAnimationIndex;

void main() {
#ifdef SKINNED
  mat4 skin = GetSkinMat(uAnimationIndex, aBonesIds, aWeights);
  mat3 normal_matrix_view = transpose(inverse(mat3(uView * uModel * skin)));
  vec4 local_pos = skin * aPosition;
#else
  mat3 normal_matrix_view = transpose(inverse(mat3(uView * uModel)));
  vec4 local_pos = aPosition;
#endif

  vs_out.tex_coords = aTexCoords;
  vs_out.view_normal = normal_matrix_view * aNormal;
  gl_Position = uProjView * uModel * local_pos;
};
