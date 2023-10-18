#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec4 aPosition;
layout(location = 8, component = 0) in uint aMatrixIndex;
layout(location = 8, component = 1) in uint aMaterialIndex;
layout(location = 8, component = 2) in uint aAnimationIndex;
layout(location = 8, component = 3) in uint aInstanceIndex;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_MATRICES readonly
#define STORAGE_INSTANCES readonly
#define STORAGE_BOXES readonly
#include "shaders/ssbo/boxes.glsl"
#include "shaders/ssbo/instances.glsl"
#include "shaders/ssbo/matrices.glsl"

vec3 GetScale(mat4 t) {
  float scale_x = length(vec3(t[0]));
  float scale_y = length(vec3(t[1]));
  float scale_z = length(vec3(t[2]));
  return vec3(scale_x, scale_y, scale_z);
}

mat3 GetRotation(mat4 t, vec3 scale) {
  mat3 rotation = mat3(t);
  rotation[0] /= scale.x;
  rotation[1] /= scale.y;
  rotation[2] /= scale.z;
  return rotation;
}

mat4 GetObbModel(Aabb box, mat4 t) {
  vec3 scale = GetScale(t);
  mat3 rotation = GetRotation(t, scale);
  // to world space
  vec3 center = vec3(t * box.center);
  vec3 extent = scale * box.extent.xyz;
  // create new model matrix
  mat4 model = mat4(1.0);
  model[0] = vec4(rotation[0], 0.0);
  model[1] = vec4(rotation[1], 0.0);
  model[2] = vec4(rotation[2], 0.0);
  model[0] *= extent.x;
  model[1] *= extent.y;
  model[2] *= extent.z;
  model[3] = vec4(center, 1.0);
  return model;
}

mat4 GetAabbModel(Aabb box, mat4 t) {
  mat3 abs_mat = mat3(t);
  abs_mat[0] = abs(abs_mat[0]);
  abs_mat[1] = abs(abs_mat[1]);
  abs_mat[2] = abs(abs_mat[2]);
  // to world space
  vec3 center = vec3(t * box.center);
  vec3 extent = vec3(abs_mat * box.extent.xyz);
  // create new model matrix
  mat4 model = mat4(1.0);
  model[0] *= extent.x;
  model[1] *= extent.y;
  model[2] *= extent.z;
  model[3] = vec4(center, 1.0);

  return model;
}

void main() {
  Instance instance = sInstances[aInstanceIndex];
  mat4 transformation = sWorld[aMatrixIndex];

  Aabb box;
  if (instance.animation_index == 0) {
    box = sStaticBoxes[instance.box_index];
  } else {
    box = sSkinnedBoxes[instance.box_index];
  }

#ifdef OBB
  mat4 model = GetObbModel(box, transformation);
#else
  mat4 model = GetAabbModel(box, transformation);
#endif

  gl_Position = uProjView * model * aPosition;
};
