#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// based on
// https://github.com/nvpro-samples/gl_ssao

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out float fFragColor[8];

// ubo
#include "shaders/ubo/engine.glsl"
#include "shaders/ubo/hbao.glsl"

// uniform
layout(location = 0) uniform vec2 uOffsetUv;

// textures
layout(binding = 0) uniform sampler2D texLinearDepth;

void main() {
  // fullscreen, pixel center
  vec2 pixel_coords = floor(gl_FragCoord.xy) * 4.0 + 0.5;
  // offsets, 1 pass: [0.5, 0.5], 2 pass: [0.5, 2.5]
  pixel_coords += uOffsetUv;
  vec2 uv = pixel_coords * uInvScreenSize;

  const int kRedChannel = 0;
  vec4 S0 = textureGather(texLinearDepth, uv, kRedChannel);
  vec4 S1 = textureGatherOffset(texLinearDepth, uv, ivec2(2, 0), kRedChannel);

  fFragColor[0] = S0.w;
  fFragColor[1] = S0.z;
  fFragColor[2] = S1.w;
  fFragColor[3] = S1.z;
  fFragColor[4] = S0.x;
  fFragColor[5] = S0.y;
  fFragColor[6] = S1.x;
  fFragColor[7] = S1.y;
}