layout(std430, binding = BIND_CROSSHAIR) readonly
    restrict buffer StorageCrosshair {
  mat4 sProjView;
  vec4 sColor;
  //
  vec2 sResolution;
  float sThickness;
  float sEmptyPad;
  //
  vec4 sVertex[];
};
