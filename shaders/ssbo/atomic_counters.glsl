// atomic operations, without 'coherent'
layout(std430, binding = BIND_ATOMIC_COUNTERS) STORAGE_ATOMIC_COUNTERS
    restrict buffer StorageAtomicCounters {
  uint sFrustumPointLightsCount;
  uint sFrustumPointShadowsCount;
  uint sClusterPointLightsCount;
  uint sFrustumInstancesDirectShadowsCount;
  uint sFrustumInstancesPointShadowsCount;
  uint sFrustumInstancesViewCount;
  uint sOcclusionInstancesViewCount;
  uint sCmdInstanceCount[MAX_MESHES];
};
