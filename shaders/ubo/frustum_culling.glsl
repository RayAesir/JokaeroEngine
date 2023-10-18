layout(std140, binding = BIND_FRUSTUM_CULLING) uniform UniformFrustumCulling {
  uvec4 uCmdOffsets[MESH_TYPES];
  //
  uint uScenePointLightsCount;
  uint uSceneMeshCount;
  uint uSceneInstanceCount;
  uint uEmptyPadFrustum0;
  // pick light
  vec4 uRayOrigin;
  vec4 uRayDir;
  // vec4 as vec3(plane.normal), plane.dist
  vec4 uFrustumPlanes[FRUSTUM_PLANES];
  uvec4 uCascadesDopTotal;
  vec4 uCascadesDopPlanes[MAX_CASCADES * DOP_PLANES];
};

uint UnpackCmdIndex(uint packed_cmd) {
  uint mesh_type = packed_cmd >> 16;
  uint local_index = packed_cmd & 0x0000FFFF;
  return uCmdOffsets[mesh_type][0] + local_index;
}

uint UnpackMeshType(uint packed_cmd) { return packed_cmd >> 16; }