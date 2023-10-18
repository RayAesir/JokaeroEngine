layout(std430,
       binding = BIND_FRUSTUM_POINT_SHADOWS) STORAGE_FRUSTUM_POINT_SHADOWS
    restrict buffer StorageFrustumPointShadows {
  mat4 sFrustumPointShadowsProjView[MAX_POINT_SHADOWS * CUBEMAP_FACES];
  vec4 sFrustumPointShadowsPosInvRad2[MAX_POINT_SHADOWS];
  vec4 sFrustumPointShadowsLayerFrustum[MAX_POINT_SHADOWS * CUBEMAP_FACES]
                                       [FRUSTUM_PLANES];
};
