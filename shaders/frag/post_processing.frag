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
#include "shaders/ubo/engine.glsl"

// textures
layout(binding = 0) uniform sampler2D texHdr;

// https://blog.johnnovak.net/2016/09/21/what-every-coder-should-know-about-gamma/
// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color

// Luminance (L or Y) is a linear measure of light (not sRGB)
// Luma (Y') is a gamma encoded, it is not linear luminance

// Photometric/Digital ITU BT.709, standard linear
float Luminance(vec3 rgb) { return dot(rgb, vec3(0.2126, 0.7152, 0.0722)); }

// Digital ITU BT.601, W3C, perceived option 1, gamma-compressed
float Luma1(vec3 srgb) { return dot(srgb, vec3(0.299, 0.587, 0.114)); }

// HSP Color Model similar to Abode Photoshop 6,
// by Darel Rex Finley, perceived option 2, gamma-compressed
float Luma2(vec3 srgb) {
  vec3 rgb2 = srgb * srgb;
  return sqrt(dot(rgb2, vec3(0.299, 0.587, 0.114)));
}

// tone mapping
vec3 Reinhard(vec3 x) { return x / (1.0 + x); }

vec3 ReinhardLuminance(vec3 x) {
  float L = Luminance(x);
  float tonemapped_luminance = L / (1.0 + L);
  x *= tonemapped_luminance / L;
  return x;
}

vec3 RomBinDaHouse(vec3 x) { return exp(-1.0 / (2.72 * x + 0.15)); }

// ACES Filmic Tone Mapping Curve
// Academy Color Encoding System (ACES)
vec3 Filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - vec3(0.004));
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

vec3 Uncharted2Tonemap(vec3 W) {
  // shoulder strength
  const float A = 0.15;
  // linear strength
  const float B = 0.50;
  // linear angle
  const float C = 0.10;
  // toe strength
  const float D = 0.20;
  // toe numerator
  const float E = 0.02;
  // toe denominator
  const float F = 0.30;
  return ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 x) {
  const vec3 kW = vec3(11.2);
  const float kExposureBias = 2.0;
  vec3 color = Uncharted2Tonemap(x * kExposureBias);
  vec3 white_scale = 1.0 / Uncharted2Tonemap(kW);
  return color * white_scale;
}

float normpdf(float x, float sigma) {
  return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

vec3 Blur() {
  // based on: https://www.shadertoy.com/view/XdfGDH
  const vec2 kTextureDims = textureSize(texHdr, 0);
  vec3 color = vec3(0.0);

  // declare stuff
  const int mSize = 9;
  const int kSize = (mSize - 1) / 2;
  float kernel[mSize];

  // create the 1-D kernel
  float Z = 0.0;
  for (int j = 0; j <= kSize; ++j) {
    kernel[kSize + j] = kernel[kSize - j] = normpdf(float(j), uBlurSigma);
  }

  // get the normalization factor (as the gaussian has been clamped)
  for (int j = 0; j < mSize; ++j) {
    Z += kernel[j];
  }

  // read out the texels
  for (int i = -kSize; i <= kSize; ++i) {
    for (int j = -kSize; j <= kSize; ++j) {
      vec2 texel = vTexCoords + (vec2(float(i), float(j)) / kTextureDims);
      color +=
          kernel[kSize + j] * kernel[kSize + i] * texture(texHdr, texel).rgb;
    }
  }

  return vec3(color / (Z * Z));
}

vec3 SrgbEncode(vec3 v) {
  return mix(12.92 * v, 1.055 * pow(v, vec3(0.41666)) - 0.055,
             lessThan(vec3(0.0031308), v));
}

void main() {
  vec3 color;

  if (uEnableBlur == 1) {
    color = Blur();
  } else {
    color = texture(texHdr, vTexCoords).rgb;
  }

  color *= uExposure;

  // HDR -> LDR
  switch (uToneMappingType) {
    case 0:
      color = Reinhard(color);
      break;
    case 1:
      color = ReinhardLuminance(color);
      break;
    case 2:
      color = RomBinDaHouse(color);
      break;
    case 3:
      color = Filmic(color);
      break;
    case 4:
      color = Uncharted2(color);
      break;
  }

  // sRGB is not the same thing as 2.2 gamma
  if (uUseSrgbEncoding == 1) {
    // sRGB luminosity encoding
    color = SrgbEncode(color);
  } else {
    // fake sRGB
    color = pow(color, vec3(1.0 / 2.2));
  }

  float alpha = 1.0;
  if (uEnableFxaa == 1) {
    alpha = Luma1(color);
  }

  // output
  fFragColor = vec4(color, alpha);
}
