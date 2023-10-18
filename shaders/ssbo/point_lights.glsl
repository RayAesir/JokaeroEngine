struct PointLight {
  uint id;
  uint empty_pad0;
  uint empty_pad1;
  uint empty_pad2;
  //
  uint props;
  float radius;
  float radius2;
  float inv_radius2;
  //
  vec4 diffuse;
  vec4 position;
};

layout(std430, binding = BIND_POINT_LIGHTS) STORAGE_POINT_LIGHTS
    restrict buffer StoragePointLights {
  PointLight sPointLights[];
};
