#version 460 core

// in
layout(location = 0) in vec3 gWorldPosition;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform sampler2D texEquirectangular;

// constants
const vec2 kInvAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 pos) {
  vec2 uv = vec2(atan(pos.z, pos.x), asin(pos.y));
  uv *= kInvAtan;
  uv += 0.5;
  return uv;
}

void main() {
  vec2 uv = SampleSphericalMap(normalize(gWorldPosition));
  vec3 color = texture(texEquirectangular, uv).rgb;
  fFragColor = vec4(color, 1.0);
}