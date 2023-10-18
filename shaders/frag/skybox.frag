#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec3 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/lighting.glsl"

// textures
layout(binding = 0) uniform samplerCube texSkybox;

// uniform
layout(location = 0) uniform float uLod;

void main() {
  vec3 color = textureLod(texSkybox, vTexCoords, uLod).rgb;
  color *= uSkybox.rgb * uSkybox.a;
  fFragColor = vec4(color, 1.0);
}