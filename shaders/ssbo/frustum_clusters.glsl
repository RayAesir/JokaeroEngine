struct ClusterAABB {
  vec4 min_point;
  vec4 max_point;
};

layout(std430, binding = BIND_FRUSTUM_CLUSTERS) STORAGE_FRUSTUM_CLUSTERS
    restrict buffer StorageFrustumClusters {
  ClusterAABB sClusterBox[];
};
