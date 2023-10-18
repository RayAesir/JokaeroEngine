#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBitangent;
layout(location = 4) in vec2 aTexCoords;
layout(location = 6) in ivec4 aBonesIds;
layout(location = 7) in vec4 aWeights;
layout(location = 8, component = 0) in uint aMatrixIndex;
layout(location = 8, component = 1) in uint aMaterialIndex;
layout(location = 8, component = 2) in uint aAnimationIndex;
layout(location = 8, component = 3) in uint aInstanceIndex;
layout(location = 9) in mat4 aMatrix;

// out
layout(location = 0) out VertexData {
  mat3 tangent_to_world;
  vec2 tex_coords;
  uint material_index;
}
vs_out;

// ssbo
#define STORAGE_VISIBILITY readonly
#define STORAGE_MATRICES readonly
#define STORAGE_ANIMATIONS readonly
#include "shaders/ssbo/animations.glsl"
#include "shaders/ssbo/matrices.glsl"
#include "shaders/ssbo/visibility.glsl"

void main() {
  mat4 model = sWorld[aMatrixIndex];

#ifdef SKINNED
  mat4 skin = GetSkinMat(aAnimationIndex, aBonesIds, aWeights);
  vec4 local_pos = skin * aPosition;
  mat3 normal_matrix = transpose(inverse(mat3(model * skin)));
#else
  vec4 local_pos = aPosition;
  mat3 normal_matrix = transpose(inverse(mat3(model)));
#endif

  vec3 N = normalize(normal_matrix * aNormal);
  vec3 T = normalize(normal_matrix * aTangent);
  vec3 B = normalize(normal_matrix * aBitangent);

  vs_out.tangent_to_world = mat3(T, B, N);
  vs_out.tex_coords = aTexCoords;
  vs_out.material_index = aMaterialIndex;
  gl_Position = aMatrix * local_pos;
};
