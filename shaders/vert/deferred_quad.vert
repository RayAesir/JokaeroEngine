#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vFrustumRay;

// ubo
#include "shaders/ubo/camera.glsl"

// constants
const vec2 kQuad[4] =
    vec2[4](vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));

void main() {
  // draw mode is GL_TRIANGLE_STRIP
  // [BottomLeft, TopLeft, BottomRight, TopRight]
  vFrustumRay = uFrustumRaysWorld[gl_VertexID].xyz;
  vec4 position = vec4(kQuad[gl_VertexID], 0.0, 1.0);
  gl_Position = position;
  vTexCoords = 0.5 * position.xy + 0.5;
}