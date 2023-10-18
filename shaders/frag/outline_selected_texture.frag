#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

// in
in VertexData {
  vec2 tex_coords;
  vec3 view_normal;
}
fs_in;

// out
layout(location = 0) out vec4 fNormalsDepth;

// ssbo
#define STORAGE_MATERIALS readonly
#include "shaders/ssbo/materials.glsl"

// uniform
layout(location = 1) uniform uint uMaterialIndex;

void main() {
#ifdef ALPHA
  Material mat = sMaterials[uMaterialIndex];
  float alpha = GetAlpha(mat, fs_in.tex_coords);
  if (alpha <= mat.alpha_threshold) discard;
#endif

  // normals
  vec3 normal = normalize(fs_in.view_normal);
  // allows to have normals on both sides of singlesided polygons
  normal = gl_FrontFacing ? normal : -normal;

  fNormalsDepth.rgb = normal;
  // Reverse-Z with clip control [1.0, 0.0] -> [0.0, 1.0]
  fNormalsDepth.a = 1.0 - gl_FragCoord.z;
}
