layout(std430, binding = BIND_FRUSTUM_POINT_LIGHTS) STORAGE_FRUSTUM_POINT_LIGHTS
    restrict buffer StorageFrustumPointLights {
  uint sFrustumPointLights[MAX_POINT_LIGHTS];
  uint sFrustumPointShadows[MAX_POINT_SHADOWS];
  float sPointLightsShadowIndex[MAX_POINT_LIGHTS];
};
