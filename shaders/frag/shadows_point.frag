#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in VertexData {
  vec4 frag_pos;
#ifdef ALPHA
  vec2 tex_coords;
  flat uint material_index;
#endif
}
fs_in;

// ssbo
#define STORAGE_MATERIALS readonly
#define STORAGE_FRUSTUM_POINT_SHADOWS readonly
#include "shaders/ssbo/frustum_point_shadows.glsl"
#include "shaders/ssbo/materials.glsl"

void main() {
#ifdef ALPHA
  Material mat = sMaterials[fs_in.material_index];
  float alpha = GetAlpha(mat, fs_in.tex_coords);
  if (alpha <= mat.alpha_threshold) discard;
#endif

  // restore the light index from layer (faster than flat interpolation)
  int light_index = gl_Layer / 6;
  vec4 shadow_data = sFrustumPointShadowsPosInvRad2[light_index];
  vec3 light_pos = shadow_data.xyz;
  float inv_radius2 = shadow_data.w;
  // world space
  vec3 frag_to_light = fs_in.frag_pos.xyz - light_pos;
  float light_dist2 = dot(frag_to_light, frag_to_light);
  // normalized [0, 1]
  gl_FragDepth = light_dist2 * inv_radius2;
}
