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
out VertexData {
  vec4 clip_pos;
  vec4 normal_pos;
}
vs_out;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/engine.glsl"

// ssbo
#define STORAGE_MATRICES readonly
#define STORAGE_ANIMATIONS readonly
#include "shaders/ssbo/animations.glsl"
#include "shaders/ssbo/matrices.glsl"

float AverageScale() {
  mat4 model = sWorld[aMatrixIndex];
  return (length(model[0]) + length(model[1]) + length(model[2])) / 3.0;
}

// no gl_Positions, we use Geometry shader
void main() {
  mat4 model = sWorld[aMatrixIndex];
#ifdef SKINNED
  mat4 skin = GetSkinMat(aAnimationIndex, aBonesIds, aWeights);
  mat4 view_model = uView * model * skin;
#else
  mat4 view_model = uView * model;
#endif

  vec4 view_pos = view_model * aPosition;
  // remove translation part
  mat3 normal_mat = transpose(inverse(mat3(view_model)));
  float avg_scale = AverageScale();
  vec3 normal = normalize(normal_mat * aNormal) *
                uDebugNormalsColorMagnitude.a * avg_scale;
  vec4 normal_pos = view_pos + vec4(normal, 0.0);
  // clip-space
  vs_out.clip_pos = uProj * view_pos;
  vs_out.normal_pos = uProj * normal_pos;
}
