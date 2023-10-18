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

// out
layout(location = 0) out VertexData {
  vec4 frag_pos;
#ifdef ALPHA
  vec2 tex_coords;
  uint material_index;
#endif
}
vs_out;

// ssbo
#define STORAGE_ANIMATIONS readonly
#define STORAGE_FRUSTUM_POINT_SHADOWS readonly
#define STORAGE_MATRICES readonly
#include "shaders/ssbo/animations.glsl"
#include "shaders/ssbo/frustum_point_shadows.glsl"
#include "shaders/ssbo/matrices.glsl"

void main() {
#ifdef SKINNED
  mat4 skin = GetSkinMat(aAnimationIndex, aBonesIds, aWeights);
  vec4 local_pos = skin * aPosition;
#else
  vec4 local_pos = aPosition;
#endif

  vec4 world_pos = sWorld[aMatrixIndex] * local_pos;
  vs_out.frag_pos = world_pos;

#ifdef ALPHA
  vs_out.tex_coords = aTexCoords;
  vs_out.material_index = aMaterialIndex;
#endif

  gl_Layer = int(aShadowIndex);
  gl_Position = sFrustumPointShadowsProjView[aShadowIndex] * world_pos;
}