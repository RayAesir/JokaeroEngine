#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/engine.glsl"

void main() { fFragColor = vec4(uDebugNormalsColorMagnitude.rgb, 1.0); }