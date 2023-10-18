#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform usampler2D texImage;

// uniform
layout(location = 0) uniform int uFormat;

// constants
const int kRGB = 0;
const int kRed = 1;
const int kGreen = 2;
const int kBlue = 3;
const int kAlpha = 4;

void main() {
  uint packed_value = texture(texImage, vTexCoords).r;
  vec4 color = unpackUnorm4x8(packed_value);

  // normals
  color.xyz = color.xyz * 2.0 - 1.0;

  vec4 result = vec4(1.0);
  switch (uFormat) {
    case kRGB:
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
  }

  fFragColor = result;
}
