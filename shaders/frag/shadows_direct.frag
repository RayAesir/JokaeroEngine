#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

#ifdef ALPHA

// in
layout(location = 0) in vec2 vTexCoords;
flat layout(location = 1) in uint vMaterialIndex;

// ssbo
#define STORAGE_MATERIALS readonly
#include "shaders/ssbo/materials.glsl"

void main() {
  Material mat = sMaterials[vMaterialIndex];
  float alpha = GetAlpha(mat, vTexCoords);
  if (alpha <= mat.alpha_threshold) discard;
}

#else

void main() {
  //
}

#endif
