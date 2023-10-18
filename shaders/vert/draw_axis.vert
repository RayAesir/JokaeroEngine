#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out vec4 vColor;

// ubo
#include "shaders/ubo/camera.glsl"

// constants
const vec4 kRed = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 kGreen = vec4(0.0, 1.0, 0.0, 1.0);
const vec4 kBlue = vec4(0.0, 0.0, 1.0, 1.0);

const vec4 kAxisPos[12] = vec4[12](
    // x, left-right
    vec4(-1.0, 0.0, 0.0, 0.0),  //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(1.0, 0.0, 0.0, 0.0),   //
    // y, bottom-top
    vec4(0.0, -1.0, 0.0, 0.0),  //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(0.0, 1.0, 0.0, 0.0),   //
    // z, forward-backward
    vec4(0.0, 0.0, -1.0, 0.0),  //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(0.0, 0.0, 0.0, 1.0),   //
    vec4(0.0, 0.0, 1.0, 0.0)    //
);

const vec4 kAxisColor[3] = vec4[3](kRed, kBlue, kGreen);

void main() {
  vColor = vec4(kAxisColor[gl_VertexID / 4]);
  vec4 pos = uProjView * vec4(kAxisPos[gl_VertexID]);
  // perspective division is performed after the vertex shader run
  // xyzw -> xyz, where z our depth, so w / w = 1.0
  // z always has maximum
  //   gl_Position = pos.xyww;
  // for reversed-z set z to 0.0
  pos.z = 0.0;
  gl_Position = pos;
}
