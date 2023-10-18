#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;

// ssbo
#define STORAGE_FX_BOXES
#include "shaders/ssbo/fx_boxes.glsl"

// ubo
#include "shaders/ubo/camera.glsl"

mat4 GetAabbModel(Aabb box) {
  // no rotation, boxes are already at world space
  mat4 model = mat4(1.0);
  // create new model matrix
  model[0] *= box.extent.x;
  model[1] *= box.extent.y;
  model[2] *= box.extent.z;
  model[3] = box.center;

  return model;
}

void main() {
  mat4 model = GetAabbModel(sFxBoxes[gl_InstanceID]);
  gl_Position = uProjView * model * aPosition;
};
