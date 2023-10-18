#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_SHOW_CAMERAS
#include "shaders/ssbo/show_cameras.glsl"

// points order
const uint kFarBottomLeft = 0;
const uint kFarTopLeft = 1;
const uint kFarTopRight = 2;
const uint kFarBottomRight = 3;
const uint kNearBottomLeft = 4;
const uint kNearTopLeft = 5;
const uint kNearTopRight = 6;
const uint kNearBottomRight = 7;
const uint kTotalPoints = 8;

// frustum
const uint kOrder[24] = uint[24](kFarBottomLeft, kFarBottomRight,  // X
                                 kFarBottomLeft, kFarTopLeft,      // Y
                                 kFarBottomLeft, kNearBottomLeft,  // Z
                                 //
                                 kFarTopRight, kFarTopLeft,      // X
                                 kFarTopRight, kFarBottomRight,  // Y
                                 kFarTopRight, kNearTopRight,    // Z
                                 //
                                 kNearTopLeft, kNearTopRight,    // X
                                 kNearTopLeft, kFarTopLeft,      // Z
                                 kNearTopLeft, kNearBottomLeft,  // Y
                                 //
                                 kNearBottomRight, kNearBottomLeft,  // X
                                 kNearBottomRight, kNearTopRight,    // Y
                                 kNearBottomRight, kFarBottomRight   // Z
);

void main() {
  // each point draw 3 times, [0, 23] -> [0, 7]
  uint point_index = uint(mod(gl_VertexID, 24));
  uint camera_index = uint(gl_VertexID / 24);
  uint global_index = camera_index * kTotalPoints + kOrder[point_index];
  gl_Position = uProjView * sCamerasPoints[global_index];
}
