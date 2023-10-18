#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out vec4 fFragColor;

// ssbo
#define STORAGE_CROSSHAIR
#include "shaders/ssbo/crosshair.glsl"

void main() { fFragColor = sColor; }