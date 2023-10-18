layout(std140, binding = BIND_LIGHTING) uniform UniformLighting {
  uint uPointLightsTotal;
  uint uPointShadowsTotal;
  uvec2 uClusterSizePx;
  //
  float uSliceScalingFactor;
  float uSliceBiasFactor;
  float uEmptyPadCluster0;
  float uEmptyPadCluster1;
  //
  vec4 uClusterDepthRange[CLUSTERS_PER_Z];
  //
  vec4 uLightDir;
  vec4 uLightDiffuse;
  vec4 uLightAmbient;
  vec4 uSkybox;
};

uint GetClusterIndex(float linear_depth, vec2 pixel_coords) {
  const uvec3 kClusterSize =
      uvec3(CLUSTERS_PER_X, CLUSTERS_PER_Y, CLUSTERS_PER_Z);
  uint cluster_z = uint(
      max(log(linear_depth) * uSliceScalingFactor + uSliceBiasFactor, 0.0));
  uvec2 cluster_xy = uvec2(pixel_coords / float(uClusterSizePx));
  uvec3 cluster_pos = uvec3(cluster_xy, cluster_z);
  return cluster_pos.z * kClusterSize.x * kClusterSize.y +  //
         cluster_pos.y * kClusterSize.x +                   //
         cluster_pos.x;
}
