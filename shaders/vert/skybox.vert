#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;

// out
// cubemap is 3D texture, use vec3
layout(location = 0) out vec3 vTexCoords;

// ubo
#include "shaders/ubo/camera.glsl"

void main() {
  vTexCoords = aPosition.xyz;
  vec4 pos = uProj * mat4(mat3(uView)) * aPosition;
  // perspective division is performed after the vertex shader run
  // xyzw -> xyz, where z our depth, so w / w = 1.0
  // z always has maximum
  //   gl_Position = pos.xyww;
  // for reversed-z set z to 0.0
  pos.z = 0.0;
  gl_Position = pos;
}
