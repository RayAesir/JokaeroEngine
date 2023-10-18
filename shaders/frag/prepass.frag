#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

// in
flat layout(location = 0) in uint vInstanceIndex;
#ifdef ALPHA
layout(location = 1) in vec2 vTexCoords;
flat layout(location = 2) in uint vMaterialIndex;
#else
layout(early_fragment_tests) in;
#endif

// out
layout(location = 0) out float fLinearDepth;
layout(location = 1) out uint fOcclusion;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_MATERIALS readonly
#include "shaders/ssbo/materials.glsl"

void main() {
#ifdef ALPHA
  Material mat = sMaterials[vMaterialIndex];
  float alpha = GetAlpha(mat, vTexCoords);
  if (alpha <= mat.alpha_threshold) discard;
#endif

  // window space [0, 1] to view space [near, far]
  // Reversed-Z or not, the B / A + Zd provides linear depth
  fLinearDepth = (uProj[3][2] / (uProj[2][2] + gl_FragCoord.z));
  fOcclusion = vInstanceIndex;
}
