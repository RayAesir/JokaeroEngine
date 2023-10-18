layout(std140, binding = BIND_CAMERA) uniform UniformCamera {
  mat4 uView;
  mat4 uProj;
  mat4 uProjInverse;
  mat4 uProjView;
  mat4 uFrustumRaysWorld;
  vec4 uCameraPos;
  vec4 uProjInfo;
  float uInvDepthRange;
};
