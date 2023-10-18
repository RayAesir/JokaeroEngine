layout(std430, binding = BIND_CLUSTER_LIGHTS) STORAGE_CLUSTER_LIGHTS
    restrict buffer StorageClusterLights {
  uvec2 sPointLightsCountOffset[CLUSTERS_TOTAL];
  uint sPointLightsIndices[];
};
