#version 460 core

// in
layout(location = 0) in vec3 gWorldPosition;

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform samplerCube texEnvironment;

// constants
const float kPi = 3.14159265359;
const float kSampleDelta = 0.025;

void main() {
  // the world vector acts as the normal of a tangent surface
  // from the origin, aligned to WorldPos
  // given this normal, calculate all incoming radiance of the environment
  vec3 normal = normalize(gWorldPosition);

  // tangent space calculation from origin point
  vec3 up = vec3(0.0, 1.0, 0.0);
  vec3 right = normalize(cross(up, normal));
  up = normalize(cross(normal, right));

  vec3 irradiance = vec3(0.0);
  float num_samples = 0.0;
  for (float phi = 0.0; phi < 2.0 * kPi; phi += kSampleDelta) {
    for (float theta = 0.0; theta < 0.5 * kPi; theta += kSampleDelta) {
      // spherical to cartesian (in tangent space)
      vec3 tangent_sample =
          vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      // tangent space to world
      vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up +
                        tangent_sample.z * normal;
      irradiance +=
          texture(texEnvironment, sample_vec).rgb * cos(theta) * sin(theta);
      ++num_samples;
    }
  }
  irradiance = kPi * irradiance * (1.0 / num_samples);
  fFragColor = vec4(irradiance, 1.0);
}