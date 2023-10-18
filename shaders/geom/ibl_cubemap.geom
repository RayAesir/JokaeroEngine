#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// geometry
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

// out
layout(location = 0) out vec3 gFragPos;

// ubo
#include "shaders/ubo/engine.glsl"

void main() {
  for (int f = 0; f < CUBEMAP_FACES; ++f) {
    gl_Layer = f;
    for (int v = 0; v < 3; ++v) {
      gFragPos = gl_in[v].gl_Position.xyz;
      gl_Position = uCubemapSpace[f] * gl_in[v].gl_Position;
      EmitVertex();
    }
    EndPrimitive();
  }
}
