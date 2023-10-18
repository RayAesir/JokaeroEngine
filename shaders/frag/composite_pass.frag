#version 460 core

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform sampler2D texAccumColor;
layout(binding = 1) uniform sampler2D texRevealAlpha;

const float kEpsilon = 0.0001;

// calculate floating point numbers equality accurately
bool IsApproximatelyEqual(float a, float b) {
  return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * kEpsilon;
}

// get the max value between three values
float Max3(vec3 v) { return max(max(v.x, v.y), v.z); }

void main() {
  float revealage = texture(texRevealAlpha, vTexCoords).r;

  // save the blending and color texture fetch cost if there is not a
  // transparent fragment
  if (IsApproximatelyEqual(revealage, 1.0)) discard;

  vec4 accum = texture(texAccumColor, vTexCoords);

  // suppress overflow
  if (isinf(Max3(abs(accum.rgb)))) accum.rgb = vec3(accum.a);

  // prevent floating point precision bug
  vec3 average_color = accum.rgb / max(accum.a, kEpsilon);

  // blend pixels
  fFragColor = vec4(average_color, revealage);
}
