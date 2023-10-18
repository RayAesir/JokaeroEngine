layout(std140, binding = BIND_ENGINE) uniform UniformEngine {
  mat4 uCubemapSpace[CUBEMAP_FACES];
  //
  vec2 uInvScreenSize;
  int uEnableHbao;
  int uEnableGtao;
  // WBOIT
  float uMeshPow;
  float uMeshClamp;
  float uParticlesPow;
  float uParticlesClamp;
  // Outline
  float uDepthThreshold;
  float uDepthNormalThreshold;
  float uDepthNormalThresholdScale;
  float uNormalThreshold;
  vec4 uOutlineColorWidth;
  // Postprocess
  float uExposure;
  int uToneMappingType;
  int uEnableBlur;
  float uBlurSigma;
  //
  int uEnableFxaa;
  int uUseSrgbEncoding;
  ivec2 uMousePos;
  // Debug Normals
  vec4 uDebugNormalsColorMagnitude;
};
