layout(std430, binding = BIND_VISIBILITY) STORAGE_VISIBILITY
    restrict buffer StorageVisibility {
  uint sInstanceVisibility[MAX_INSTANCES];
  uvec2 sPointShadowsCountOffset[];
};
