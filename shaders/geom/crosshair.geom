#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

// ssbo
#define STORAGE_CROSSHAIR
#include "shaders/ssbo/crosshair.glsl"

void main() {
  vec4 p1 = gl_in[0].gl_Position;
  vec4 p2 = gl_in[1].gl_Position;

  vec2 dir = normalize((p2.xy - p1.xy) * sResolution);
  vec2 offset = vec2(-dir.y, dir.x) * sThickness / sResolution;

  gl_Position = p1 + vec4(offset * p1.w, 0.0, 0.0);
  EmitVertex();
  gl_Position = p1 - vec4(offset * p1.w, 0.0, 0.0);
  EmitVertex();
  gl_Position = p2 + vec4(offset * p2.w, 0.0, 0.0);
  EmitVertex();
  gl_Position = p2 - vec4(offset * p2.w, 0.0, 0.0);
  EmitVertex();

  EndPrimitive();
}
