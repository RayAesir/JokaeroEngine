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
#include "shaders/ubo/frustum_culling.glsl"

// ssbo
#define STORAGE_POINT_LIGHTS readonly
#include "shaders/ssbo/point_lights.glsl"

// uniform
layout(location = 0) uniform float uAlpha;

// select light
bool TestRaySphere(vec3 sphere_center, float sphere_radius2) {
  vec3 co = sphere_center - uRayOrigin.xyz;
  float b = dot(co, uRayDir.xyz);
  float c = dot(co, co) - sphere_radius2;
  float discriminant = b * b - c;
  return (discriminant >= 0.0) ? true : false;
}

void main() {
  const float kOutlineWidth = 0.2;
  const vec3 kAddBloomWhite = vec3(3.0) / uAlpha;
  PointLight light = sPointLights[vLightIndex];

  vec3 L = normalize(vFragPos.xyz - light.position.xyz);
  vec3 V = normalize(vFragPos.xyz - uCameraPos.xyz);
  float angle = abs(dot(L, V));

  vec3 final_color = light.diffuse.rgb * angle;
  if (TestRaySphere(light.position.xyz, light.radius2)) {
    if (angle < kOutlineWidth) {
      final_color += kAddBloomWhite;
    }
  }
  fFragColor = vec4(final_color, uAlpha);
}