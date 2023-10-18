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
#include "shaders/ubo/camera.glsl"

// textures
layout(binding = 0) uniform sampler2D texImage;

// uniform
layout(location = 0) uniform int uFormat;
layout(location = 1) uniform float uMipLevel;

// constants
const int kRgb = 0;
const int kRed = 1;
const int kGreen = 2;
const int kBlue = 3;
const int kAlpha = 4;
const int kDepthPrepass = 5;
const int kDepthOutline = 6;
const int kRgbHdr = 7;

vec3 Reinhard(vec3 x) { return x / (1.0 + x); }

void main() {
  vec4 color = textureLod(texImage, vTexCoords, uMipLevel);

  vec4 result = vec4(1.0);
  switch (uFormat) {
    case kRgb:
      result = vec4(color.rgb, 1.0);
      break;
    case kRed:
      result = vec4(vec3(color.r), 1.0);
      break;
    case kGreen:
      result = vec4(vec3(color.g), 1.0);
      break;
    case kBlue:
      result = vec4(vec3(color.b), 1.0);
      break;
    case kAlpha:
      result = vec4(vec3(color.a), 1.0);
      break;
    case kDepthPrepass:
      // normalize depth [0.0, 1.0]
      result = vec4(vec3(color.r * uInvDepthRange), 1.0);
      break;
    case kDepthOutline:
      // Reverse-Z to normalized straight depth
      float linear_depth = uProj[3][2] / (uProj[2][2] + (1.0 - color.a));
      linear_depth *= uInvDepthRange;
      result = vec4(vec3(1.0 - linear_depth), 1.0);
      break;
    case kRgbHdr:
      result = vec4(pow(Reinhard(color.rgb), vec3(1.0 / 2.2)), 1.0);
      break;
  }

  fFragColor = result;
}