#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in VertexData {
  mat3 tangent_to_world;
  vec2 tex_coords;
  flat uint material_index;
}
fs_in;

// out
layout(location = 0) out vec2 fNormals;
layout(location = 1) out vec4 fColor;
layout(location = 2) out vec2 fPbr;

// ssbo
#define STORAGE_MATERIALS readonly
#include "shaders/ssbo/materials.glsl"

#ifndef ALPHA
layout(early_fragment_tests) in;
#endif

// Reduce specular aliasing by producing a modified roughness value
// Tokuyoshi et al. 2019. Improved Geometric Specular Antialiasing.
// http://www.jp.square-enix.com/tech/library/pdf/ImprovedGeometricSpecularAA.pdf
float SpecularAntiAliasing(vec3 N, float a) {
  // normal-based isotropic filtering
  // this is originally meant for deferred rendering but is a bit simpler to
  // implement than the forward version saves us from calculating uv offsets and
  // sampling textures for every light

  // squared std dev of pixel filter kernel (in pixels)
  const float kSigma2 = 0.25;
  // clamping threshold
  const float kKappa = 0.18;
  vec3 dndu = dFdx(N);
  vec3 dndv = dFdy(N);
  float variance = kSigma2 * (dot(dndu, dndu) + dot(dndv, dndv));
  float kernel_roughness2 = min(2.0 * variance, kKappa);
  return clamp(a + kernel_roughness2, 0.0, 1.0);
}

vec2 PackNormal(vec3 normal) {
  float f = sqrt(8.0 * -normal.z + 8.0);
  return normal.xy / f + 0.5;
}

void main() {
  Material mat = sMaterials[fs_in.material_index];
  vec4 color = GetDiffuse(mat, fs_in.tex_coords);
#ifdef ALPHA
  // alpha cutout
  if (color.a <= mat.alpha_threshold) discard;
#endif

  vec3 normal = GetNormals(mat, fs_in.tex_coords);
  normal = normalize(fs_in.tangent_to_world * normal);
#ifdef ALPHA
  // allows to have normals on both sides of singlesided polygons
  normal = gl_FrontFacing ? normal : -normal;
#endif

  float roughness = GetRoughness(mat, fs_in.tex_coords);
  float perceptual_roughness = max(roughness * roughness, 0.01);
  roughness = SpecularAntiAliasing(normal, perceptual_roughness);
  float metallic = GetMetallic(mat, fs_in.tex_coords);

  fColor = vec4(color.rgb, 1.0);
  fNormals = PackNormal(normal);
  fPbr = vec2(roughness, metallic);
}
