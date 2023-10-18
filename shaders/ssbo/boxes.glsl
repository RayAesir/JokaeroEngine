struct Aabb {
  vec4 center;
  vec4 extent;
};

layout(std430, binding = BIND_STATIC_BOXES) STORAGE_BOXES
    restrict buffer StorageStaticBoxes {
  Aabb sStaticBoxes[];
};

layout(std430, binding = BIND_SKINNED_BOXES) STORAGE_BOXES
    restrict buffer StorageSkinnedBoxes {
  Aabb sSkinnedBoxes[];
};
