layout(std430, binding = BIND_PICK_ENTITY) STORAGE_PICK_ENTITY
    restrict buffer StoragePickEntity {
  uint sPointLightBufferId;  // show light
  uint sPointLightId;        // select in the Scene
  //
  uint sOpaqueInstanceId;  // check if none
  uint sOpaqueObjectId;    // select in the Scene
  float sOpaqueDist;
  //
  uint sTransparentInstanceId;
  uint sTransparentObjectId;
  float sTransparentDist;
};
