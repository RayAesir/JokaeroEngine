#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// based on
// https://github.com/nvpro-samples/gl_ssao

// header
#include "shaders/header.glsl"

// in
in vec2 vTexCoords;

// out
layout(location = 0) out vec2 fFragColor;

// ubo
#include "shaders/ubo/engine.glsl"
#include "shaders/ubo/hbao.glsl"

// textures
layout(binding = 0) uniform sampler2D texAoDepth;

// constants
const float kKernelRadius = 3.0;
const float kBlurSigma = float(kKernelRadius) * 0.5;
const float kBlurFalloff = 1.0 / (2.0 * kBlurSigma * kBlurSigma);

float BlurFunction(vec2 uv, float radius, float center_ao, float center_depth,
                   inout float weight_total) {
  vec2 ao_depth = texture(texAoDepth, uv).rg;
  float ao = ao_depth.r;
  float depth = ao_depth.g;

  float depth_diff = (depth - center_depth) * uBlurSharpness;
  float radius2 = radius * radius;
  float depth_diff2 = depth_diff * depth_diff;
  float weight = exp2(-radius2 * kBlurFalloff - depth_diff2);
  weight_total += weight;

  return ao * weight;
}

void main() {
#ifdef HORIZONTAL
  vec2 inv_res_dir = vec2(uInvScreenSize.x, 0.0);
#else
  vec2 inv_res_dir = vec2(0.0, uInvScreenSize.y);
#endif

  vec2 ao_depth = texture(texAoDepth, vTexCoords).rg;
  float center_ao = ao_depth.r;
  float center_depth = ao_depth.g;

  float ao_total = center_ao;
  float weight_total = 1.0;

  for (float r = 1; r <= kKernelRadius; ++r) {
    vec2 uv = vTexCoords + inv_res_dir * r;
    ao_total += BlurFunction(uv, r, center_ao, center_depth, weight_total);
  }

  for (float r = 1; r <= kKernelRadius; ++r) {
    vec2 uv = vTexCoords - inv_res_dir * r;
    ao_total += BlurFunction(uv, r, center_ao, center_depth, weight_total);
  }

  float ao = ao_total / weight_total;
  fFragColor = vec2(ao, center_depth);
}