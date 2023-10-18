layout(std140, binding = BIND_HBAO) uniform UniformHbao {
  vec2 uInvQuarterResolution;
  float uRadiusToScreen;
  float uNegInvSqrRadius;
  //
  float uBias;
  float uAoMultiplier;
  float uExponent;
  float uBlurSharpness;
  //
  vec4 uFloat2Offsets[16];
  vec4 uJitters[16];
};
