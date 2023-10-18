#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform samplerCubeArray texShadows;

// uniform
layout(location = 0) uniform int uShadowIndex;

// constants
const float kPi = 3.14159265359;

void main() {
  float phi = vTexCoords.x * kPi * 2.0;
  float theta = (-vTexCoords.y + 0.5) * kPi;
  vec3 dir = vec3(sin(phi) * cos(theta), sin(theta), sin(phi) * cos(theta));

  vec4 P = vec4(dir, float(uShadowIndex));
  // point lights with normalized linear depth
  float depth = texture(texShadows, P).r;

  fFragColor = vec4(vec3(depth), 1.0);
}
