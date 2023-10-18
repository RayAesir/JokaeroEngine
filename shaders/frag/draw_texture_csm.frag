#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform sampler2DArray texShadows;

// uniform
layout(location = 0) uniform int uCascade;

void main() {
  vec3 P = vec3(vTexCoords, float(uCascade));
  // ortho projection - linear depth
  float depth = texture(texShadows, P).r;

  // black and white
  fFragColor = vec4(vec3(depth), 1.0);
}
