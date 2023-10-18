const float kInvPi = 0.31830988618;

struct PBR {
  vec3 diffuse;
  float alpha;
  //
  vec3 normal;
  float metallic;
  //
  vec3 F0;
  float a;
};

// Physically based shading
// Metallic + roughness workflow (GLTF 2.0 core material spec)
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#metallic-roughness-material
// https://google.github.io/filament/Filament.md.html
// https://learnopengl.com/PBR/Lighting

// Schlick approximation to Fresnel equation
vec3 FresnelSchlick(float VdotH, vec3 F0) {
  float f = pow(1.0 - VdotH, 5.0);
  return f + F0 * (1.0 - f);
}

// Normal Distribution Function
// (aka specular distribution)
// distribution of microfacets

// Bruce Walter et al. 2007. Microfacet Models for Refraction through Rough
// Surfaces. equivalent to Trowbridge-Reitz
float DistributionGGX(float NdotH, float roughness) {
  float a = NdotH * roughness;
  float k = roughness / (1.0 - NdotH * NdotH + a * a);
  return k * k * kInvPi;
}

// Visibility function
// = Geometric Shadowing/Masking Function G, divided by the denominator of the
// Cook-Torrance BRDF (4 NdotV NdotL) G is the probability of the microfacet
// being visible in the outgoing (masking) or incoming (shadowing) direction

// Heitz 2014.
// Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs.
// http://jcgt.org/published/0003/02/03/paper.pdf based on
// height-correlated Smith-GGX
float GeometrySmithGGXCorrelated(float NdotV, float NdotL, float a) {
  float a2 = a * a;
  float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
  float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
  return 0.5 / (GGXV + GGXL);
}

// Lambertian diffuse BRDF
// uniform color
float FdLambert() {
  // normalize to conserve energy
  // cos integrates to pi over the hemisphere
  // incoming light is multiplied by cos and BRDF
  return kInvPi;
}

// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-b-brdf-implementation
vec3 BRDF(vec3 V, vec3 L, vec3 N, float NdotV, float NdotL, PBR pbr) {
  // V is the normalized vector from the shading location to the eye
  // L is the normalized vector from the shading location to the light
  // N is the surface normal in the same space as the above values
  // H is the half vector, where H = normalize(L+V)
  // NdotL is the incoming (incident) angle at which the light hits the surface

  vec3 H = normalize(L + V);
  float NdotH = max(dot(N, H), 0.0);
  float VdotH = max(dot(V, H), 0.0);

  // specular part
  float D = DistributionGGX(NdotH, pbr.a);
  vec3 F = FresnelSchlick(VdotH, pbr.F0);
  float G = GeometrySmithGGXCorrelated(NdotV, NdotL, pbr.a);
  vec3 Fr = D * F * G;

  // diffuse part
  vec3 Fd = pbr.diffuse * FdLambert();
  vec3 kD = vec3(1.0) - F;

  return kD * Fd + Fr;
}

// by Sebastien Lagarde
vec3 FresnelSchlickRoughness(float NdotV, vec3 F0, float a) {
  vec3 R = max(vec3(1.0 - a), F0) - F0;
  // black dots without clamp
  float f = pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
  return F0 + R * f;
}
