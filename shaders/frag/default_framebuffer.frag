#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform sampler2D texLdrLuma;

void main() {
  vec3 color = texture(texLdrLuma, vTexCoords).rgb;
  fFragColor = vec4(color, 1.0);
}
