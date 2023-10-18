layout(std140, binding = BIND_GTAO) uniform UniformGtao {
  float uSliceCount;
  float uStepsPerSlice;
  vec2 uNdcToViewMulPixelSize;
  //
  vec2 uNdcToViewMul;
  vec2 uNdcToViewAdd;
  //
  float uEffectRadius;
  float uEffectFalloffRange;
  float uRadiusMultiplier;
  float uFinalValuePower;
  //
  float uSampleDistributionPower;
  float uThinOccluderCompensation;
  float uDenoiseBlurBeta;
  float uEmptyPadGtao0;
};
