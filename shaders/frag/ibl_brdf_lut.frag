#version 460 core

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec2 fFragColor;

// constants
const float kPi = 3.14159265359;

// efficient VanDerCorpus calculation
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 Hammersley2D(uint i, uint N) {
  // Radical inverse based on
  // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
  uint bits = (i << 16u) | (i >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  // / 0x100000000
  float ri = float(bits) * 2.3283064365386963e-10;
  return vec2(float(i) / float(N), ri);
}

// Based on Karis 2014
// Xi - low-discrepancy sequence value
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
  float a = roughness * roughness;

  // sample in spherical coordinates
  float phi = 2.0 * kPi * Xi.x;
  float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

  // from spherical coordinates to cartesian coordinates - halfway vector
  vec3 H;
  H.x = cos(phi) * sin_theta;
  H.y = sin(phi) * sin_theta;
  H.z = cos_theta;

  // from tangent-space H vector to world-space sample vector
  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);

  vec3 sample_vec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sample_vec);
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float GeometrySmithGGXCorrelated(float NdotV, float NdotL, float a) {
  float a2 = pow(a, 4.0);
  float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
  float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
  return 0.5 / (GGXV + GGXL);
}

vec2 IntegrateBRDF(float NdotV, float roughness) {
  vec3 V;
  V.x = sqrt(1.0 - NdotV * NdotV);
  V.y = 0.0;
  V.z = NdotV;

  float A = 0.0;
  float B = 0.0;

  // N points straight upwards for this integration
  const vec3 N = vec3(0.0, 0.0, 1.0);

  const uint kSampleCount = 1024;
  for (uint i = 0; i < kSampleCount; ++i) {
    // generates a sample vector that's biased towards the
    // preferred alignment direction (importance sampling)
    vec2 Xi = Hammersley2D(i, kSampleCount);
    // sample microfacet direction
    vec3 H = ImportanceSampleGGX(Xi, N, roughness);
    // get the light direction
    vec3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(L.z, 0.0);
    float NdotH = max(H.z, 0.0);
    float VdotH = max(dot(V, H), 0.0);

    if (NdotL > 0.0) {
      float G = GeometrySmithGGXCorrelated(NdotV, NdotL, roughness);
      float G_Vis = (G * VdotH * NdotL) / NdotH;
      float Fc = pow(1.0 - VdotH, 5.0);

      A += (1.0 - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }

  return 4.0 * vec2(A, B) / float(kSampleCount);
}

void main() {
  vec2 integrated_BRDF = IntegrateBRDF(vTexCoords.x, vTexCoords.y);
  fFragColor = integrated_BRDF;
}