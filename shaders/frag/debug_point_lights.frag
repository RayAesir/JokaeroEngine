#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 vFragPos;
flat layout(location = 1) in uint vLightIndex;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_POINT_LIGHTS readonly
#include "shaders/ssbo/point_lights.glsl"

// uniform
layout(location = 0) uniform float uAlpha;

void main() {
  PointLight light = sPointLights[vLightIndex];

  vec3 L = normalize(vFragPos.xyz - light.position.xyz);
  vec3 V = normalize(vFragPos.xyz - uCameraPos.xyz);
  float angle = abs(dot(L, V));

  vec3 final_color = light.diffuse.rgb * angle;
  fFragColor = vec4(final_color, uAlpha);
}