#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// ssbo
#define STORAGE_CROSSHAIR readonly
#include "shaders/ssbo/crosshair.glsl"

void main() { gl_Position = sProjView * sVertex[gl_VertexID]; }