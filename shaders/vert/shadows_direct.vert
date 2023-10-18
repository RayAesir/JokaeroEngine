#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_shader_viewport_layer_array : enable

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;
layout(location = 4) in vec2 aTexCoords;
layout(location = 6) in ivec4 aBonesIds;
layout(location = 7) in vec4 aWeights;
layout(location = 8, component = 0) in uint aMatrixIndex;
layout(location = 8, component = 1) in uint aMaterialIndex;
layout(location = 8, component = 2) in uint aAnimationIndex;
layout(location = 8, component = 3) in uint aShadowIndex;
layout(location = 9) in mat4 aMatrix;

// out
#ifdef ALPHA
layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out uint vMaterialIndex;
#endif

// ubo
#include "shaders/ubo/shadows.glsl"

// ssbo
#define STORAGE_ANIMATIONS readonly
#include "shaders/ssbo/animations.glsl"

void main() {
#ifdef SKINNED
  mat4 skin = GetSkinMat(aAnimationIndex, aBonesIds, aWeights);
  vec4 local_pos = skin * aPosition;
#else
  vec4 local_pos = aPosition;
#endif

#ifdef ALPHA
  vTexCoords = aTexCoords;
  vMaterialIndex = aMaterialIndex;
#endif

  gl_Layer = int(aShadowIndex);
  gl_Position = aMatrix * local_pos;
}
