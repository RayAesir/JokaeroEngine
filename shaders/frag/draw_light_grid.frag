#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/lighting.glsl"

// ssbo
#define STORAGE_CLUSTER_LIGHTS readonly
#include "shaders/ssbo/cluster_lights.glsl"

// textures
layout(binding = 0) uniform sampler2D texLinearDepth;

// uniform
layout(location = 0) uniform float uLightGridDivider;

// fifth-order polynomial approximation of Turbo based on:
// https://observablehq.com/@mbostock/turbo
vec3 ColorMap(float x) {
  const vec3 c1 = vec3(0.1357, 0.0914, 0.1067);
  const vec3 c2 = vec3(4.5974, 2.1856, 12.5925);
  const vec3 c3 = vec3(42.3277, 4.8052, 60.1097);
  const vec3 c4 = vec3(130.5887, 14.0195, 109.0745);
  const vec3 c5 = vec3(150.5666, 4.2109, 88.5066);
  const vec3 c6 = vec3(58.1375, 2.7747, 26.8183);

  float r = c1.r + x * (c2.r - x * (c3.r - x * (c4.r - x * (c5.r - x * c6.r))));
  float g = c1.g + x * (c2.g + x * (c3.g - x * (c4.g - x * (c5.g + x * c6.g))));
  float b = c1.b + x * (c2.b - x * (c3.b - x * (c4.b - x * (c5.b - x * c6.b))));

  return vec3(r, g, b);
}

void main() {
  float linear_depth = texture(texLinearDepth, vTexCoords).r;
  uint cluster_index = GetClusterIndex(linear_depth, gl_FragCoord.xy);
  uint lights_count = sPointLightsCountOffset[cluster_index].x;

  vec3 color = ColorMap(float(lights_count) / uLightGridDivider);
  fFragColor = vec4(color, 1.0);
}
