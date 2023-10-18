#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;

// out
layout(location = 0) out vec4 vFragPos;
layout(location = 1) out uint vLightIndex;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_POINT_LIGHTS readonly
#define STORAGE_FRUSTUM_POINT_LIGHTS readonly
#include "shaders/ssbo/frustum_point_lights.glsl"
#include "shaders/ssbo/point_lights.glsl"

// uniform
layout(location = 0) uniform float uAlpha;

void main() {
  uint light_index = sFrustumPointLights[gl_InstanceID];
  PointLight light = sPointLights[light_index];

  mat4 model = mat4(1.0);
  // scale by radius
  model[0] *= light.radius;
  model[1] *= light.radius;
  model[2] *= light.radius;
  // set position
  model[3] = light.position;

  vec4 world_pos = model * aPosition;
  vFragPos = world_pos;
  vLightIndex = light_index;
  gl_Position = uProjView * world_pos;
};
