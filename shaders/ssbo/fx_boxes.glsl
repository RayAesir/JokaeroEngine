struct Aabb {
  vec4 center;
  vec4 extent;
};

layout(std430, binding = BIND_FX_BOXES) readonly buffer StorageFxBoxes {
  Aabb sFxBoxes[];
};
