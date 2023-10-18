#version 460 core

// in
layout(location = 0) in vec3 gWorldPosition;

// out
layout(location = 0) out vec4 fFragColor;

// uniform
layout(location = 0) uniform float uRoughness;

// textures
layout(binding = 0) uniform samplerCube texEnvironment;

const float kPi = 3.14159265359;

// normal distribution function
// Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  //
  float numerator = a2;
  float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
  denominator = kPi * denominator * denominator;
  return numerator / denominator;
}

// efficient VanDerCorpus calculation.
float RadicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  // / 0x100000000
  return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
  return vec2(float(i) / float(N), RadicalInverse(i));
}

// Xi - low-discrepancy sequence value
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
  float a = roughness * roughness;

  float phi = 2.0 * kPi * Xi.x;
  float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

  // from spherical coordinates to cartesian coordinates - halfway vector
  vec3 H;
  H.x = cos(phi) * sin_theta;
  H.y = sin(phi) * sin_theta;
  H.z = cos_theta;

  // from tangent-space halfway vector to world-space sample vector
  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);

  vec3 sample_vec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sample_vec);
}

void main() {
  vec3 N = normalize(gWorldPosition);

  // make the simplyfying assumption that V equals R equals the normal
  vec3 R = N;
  vec3 V = R;

  const uint SAMPLE_COUNT = 1024u;
  vec3 prefiltered_color = vec3(0.0);
  float total_weight = 0.0;

  for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
    // generates a sample vector that's biased towards the preferred alignment
    // direction (importance sampling).
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
    vec3 H = ImportanceSampleGGX(Xi, N, uRoughness);
    vec3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if (NdotL > 0.0) {
      // sample from the environment's mip level based on roughness/pdf
      float D = DistributionGGX(N, H, uRoughness);
      float NdotH = max(dot(N, H), 0.0);
      float HdotV = max(dot(H, V), 0.0);
      float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

      // resolution of source cubemap (per face)
      ivec2 env_size = textureSize(texEnvironment, 0);
      float resolution = float(env_size.x);
      float saTexel = 4.0 * kPi / (6.0 * resolution * resolution);
      float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

      float mip_level =
          uRoughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

      prefiltered_color += textureLod(texEnvironment, L, mip_level).rgb * NdotL;
      total_weight += NdotL;
    }
  }

  prefiltered_color = prefiltered_color / total_weight;

  fFragColor = vec4(prefiltered_color, 1.0);
}