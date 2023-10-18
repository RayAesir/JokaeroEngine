layout(std140, binding = BIND_SHADOWS) uniform UniformShadows {
  mat4 uShadowSpace[4];
  mat4 uShadowSpaceBias[4];
  vec4 uShadowFilterRadiusUV;
  vec4 uShadowSplitsLinear;
  //
  uint uNumCascades;
  uint uPcfSamples;
  float uPointFilterRadiusUV;
  float uEmptyPadShadows0;
  //
  vec4 uPoisson[128];
};
