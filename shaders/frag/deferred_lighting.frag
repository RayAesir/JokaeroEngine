#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFrustumRay;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/engine.glsl"
#include "shaders/ubo/lighting.glsl"
#include "shaders/ubo/shadows.glsl"

// ssbo
#define STORAGE_POINT_LIGHTS readonly
#define STORAGE_FRUSTUM_POINT_LIGHTS readonly
#define STORAGE_CLUSTER_LIGHTS readonly
#include "shaders/ssbo/cluster_lights.glsl"
#include "shaders/ssbo/frustum_point_lights.glsl"
#include "shaders/ssbo/point_lights.glsl"

// textures
layout(binding = 0) uniform sampler2D texLinearDepth;

layout(binding = 1) uniform sampler2D texNormals;
layout(binding = 2) uniform sampler2D texColor;
layout(binding = 3) uniform sampler2D texPbr;

layout(binding = 4) uniform samplerCube texIrradiance;
layout(binding = 5) uniform samplerCube texPrefilter;
layout(binding = 6) uniform sampler2D texBrdfLut;

layout(binding = 7) uniform sampler2DArrayShadow texShadowsPcf;
layout(binding = 8) uniform samplerCubeArrayShadow texPointShadowsPcf;
layout(binding = 9) uniform sampler3D texRandomAngles;

layout(binding = 10) uniform sampler2D texHbao;
layout(binding = 11) uniform usampler2D texGtao;

// functions
#include "shaders/pbr.glsl"

vec3 UnpackNormal(vec2 encoded) {
  vec2 fenc = encoded * 4.0 - 2.0;
  float f = dot(fenc, fenc);
  float g = sqrt(1.0 - f * 0.25);
  return vec3(fenc * g, -(1.0 - f * 0.5));
}

PBR GetMaterialDeferred(vec2 uv) {
  const vec3 kDielectricSpecular = vec3(0.04);
  const vec3 kBlack = vec3(0.0);

  vec4 albedo = texture(texColor, uv);
  vec2 normal = texture(texNormals, uv).xy;
  vec2 mask1 = texture(texPbr, uv).rg;
  float perceptual_roughness = mask1.r;
  float metallic = mask1.g;

  PBR pbr;
  // metals have no diffuse reflection so the albedo value stores F0
  // (reflectance at normal incidence) instead dielectrics are assumed to have
  // F0 = 0.04 which equals an IOR of 1.5
  pbr.diffuse =
      mix(albedo.rgb * (vec3(1.0) - kDielectricSpecular), kBlack, metallic);
  pbr.alpha = albedo.a;

  pbr.normal = UnpackNormal(normal);

  // Fresnel reflectance
  pbr.F0 = mix(kDielectricSpecular, albedo.rgb, metallic);
  pbr.a = perceptual_roughness;

  pbr.metallic = metallic;

  return pbr;
}

vec3 RotateDirection(vec3 dir, vec2 angles, float weight) {
  const float kHalfPi = 3.14159265359 / 2.0;
  // azimuth and elevation, [-1.0, 1.0] to [-Pi/2, Pi/2]
  float az = kHalfPi * angles.x;
  float el = kHalfPi * angles.y;
  vec3 rotation = vec3(sin(el) * cos(az), cos(el), sin(el) * sin(az));
  // the vectors do not have to be normalized
  return mix(dir, rotation, weight);
}

float PointShadowPCF(float shadow_index, float norm_dist, vec3 frag_to_light,
                     vec3 world_pos, float NdotL) {
  float bias = max(1e-4 * (1.0 - NdotL), 1e-6);
  float z_receiver = norm_dist - bias;
  if (z_receiver > 1.0) return 1.0;

  vec2 random_rotation = texture(texRandomAngles, world_pos).rg;

  float sum = 0.0;
  for (int i = 0; i < uPcfSamples; ++i) {
    vec2 offset = uPoisson[i].xy;
    offset = vec2(random_rotation.x * offset.x - random_rotation.y * offset.y,
                  random_rotation.y * offset.x + random_rotation.x * offset.y);

    vec3 sample_dir =
        RotateDirection(frag_to_light, offset, uPointFilterRadiusUV);
    sum +=
        texture(texPointShadowsPcf, vec4(sample_dir, shadow_index), z_receiver);
  }

  return sum / float(uPcfSamples);
}

float SmoothAttenuation(float norm_dist, float dist2) {
  // square falloff
  float smooth_factor = max(1.0 - norm_dist * norm_dist, 0.0);
  return (smooth_factor * smooth_factor) / max(dist2, 1e-5);
}

// light loop optimization:
// compute light attenuation
// test N dot L
// compute shadow
vec3 ProcessPointLights(float linear_depth, vec3 world_pos, vec3 V, vec3 N,
                        float NdotV, PBR pbr) {
  vec3 final_color = vec3(0.0);

  // extract the lights
  uint cluster_index = GetClusterIndex(linear_depth, gl_FragCoord.xy);
  uint lights_count = sPointLightsCountOffset[cluster_index][0];
  uint offset = sPointLightsCountOffset[cluster_index][1];

  for (uint index = 0; index < lights_count; ++index) {
    uint light_index = sPointLightsIndices[offset + index];
    PointLight light = sPointLights[light_index];

    vec3 frag_to_light = world_pos - light.position.xyz;
    // dot(v, v) * inv_radius2 is faster than length()
    float dist2 = dot(frag_to_light, frag_to_light);
    float norm_dist = dist2 * light.inv_radius2;
    float attenuation = SmoothAttenuation(norm_dist, dist2);

    if (attenuation > 0.0) {
      vec3 L = normalize(light.position.xyz - world_pos);
      float NdotL = dot(N, L);
      if (NdotL > 0.0) {
        // lights are sorted in the strict order
        if ((light.props & CAST_SHADOWS) > 0) {
          float shadow_index = sPointLightsShadowIndex[light_index];
          float shadow = PointShadowPCF(shadow_index, norm_dist, frag_to_light,
                                        world_pos, NdotL);
          if (shadow > 0.0) {
            vec3 radiance = light.diffuse.rgb * light.diffuse.w * attenuation *
                            NdotL * shadow;
            final_color += BRDF(V, L, N, NdotV, NdotL, pbr) * radiance;
          }
        } else {
          vec3 radiance =
              light.diffuse.rgb * light.diffuse.w * attenuation * NdotL;
          final_color += BRDF(V, L, N, NdotV, NdotL, pbr) * radiance;
        }
        //
      }
    }
  }

  return final_color;
}

uint GetCascade(float linear_depth) {
  uint cascade = 0;
  for (uint i = 0; i < uNumCascades - 1; ++i) {
    if (linear_depth > uShadowSplitsLinear[i]) {
      cascade = i + 1;
    } else {
      break;
    }
  }
  return cascade;
}

float DirShadowPCF(uint cascade, vec4 world_pos, float NdotL) {
  // premultiplied bias to [0,1] range
  // orthogonal matrix (w = 1.0), no perspective division
  vec4 proj_coords = uShadowSpaceBias[cascade] * world_pos;
  vec2 uv = proj_coords.xy;
  float bias = max(1e-4 * (1.0 - NdotL), 1e-6);
  float z_receiver = proj_coords.z - bias;
  if (z_receiver > 1.0) return 1.0;

  vec2 random_rotation = texture(texRandomAngles, world_pos.xyz).rg;
  float sum = 0.0;
  for (uint i = 0; i < uPcfSamples; ++i) {
    vec2 offset = uPoisson[i].xy;
    offset = vec2(random_rotation.x * offset.x - random_rotation.y * offset.y,
                  random_rotation.y * offset.x + random_rotation.x * offset.y);
    offset *= uShadowFilterRadiusUV[cascade];

    sum +=
        texture(texShadowsPcf, vec4(uv + offset, float(cascade), z_receiver));
  }

  return sum / float(uPcfSamples);
}

vec3 ReconstructWorldPos(vec3 frustum_ray, float linear_depth) {
  vec3 normalized_depth = vec3(linear_depth * uInvDepthRange);
  return frustum_ray * normalized_depth + uCameraPos.xyz;
}

// Account for multiple scattering across microfacets
// Computes a scaling factor for the BRDF Turquin. 2018.
// Practical multiple scattering compensation for microfacet models.
// https://blog.selfshadow.com/publications/turquin/ms_comp_final.pdf
// https://bruop.github.io/ibl/
vec3 AmbientBRDF(vec3 V, vec3 N, float NdotV, PBR pbr) {
  vec3 F = FresnelSchlickRoughness(NdotV, pbr.F0, pbr.a);

  // sample both the prefilter map and the BRDF lut and combine
  // them together to get the IBL specular part
  const float kMaxReflectionLod = 4.0;
  vec3 R = reflect(-V, N);
  vec3 radiance = textureLod(texPrefilter, R, pbr.a * kMaxReflectionLod).rgb;
  vec2 Fab = texture(texBrdfLut, vec2(NdotV, pbr.a)).xy;
  vec3 Fss = F * Fab.x + Fab.y;
  vec3 Fr = radiance * Fss;

  // multiple scattering, from Fdez-Aguera
  float Ems = 1.0 - (Fab.x + Fab.y);
  vec3 Favg = pbr.F0 + (vec3(1.0) - pbr.F0) / 21.0;
  vec3 Fms = Ems * Fss * Favg / (vec3(1.0) - Favg * Ems);
  vec3 kD = pbr.diffuse * (1.0 - Fss - Fms);

  // diffuse part
  vec3 irradiance = texture(texIrradiance, N).rgb;
  vec3 Fd = (Fms + kD) * irradiance;

  return Fd + Fr;
}

void main() {
  // the one texture sampling per all lights
  PBR pbr = GetMaterialDeferred(vTexCoords);
  vec3 N = pbr.normal;
  float linear_depth = texture(texLinearDepth, vTexCoords).r;

  vec3 world_pos = ReconstructWorldPos(vFrustumRay, linear_depth);
  vec3 V = normalize(uCameraPos.xyz - world_pos);
  vec3 L = uLightDir.xyz;
  float NdotV = abs(dot(N, V)) + 1e-5;

  // ambient lighting, IBL
  vec3 radiance_ambient = uLightAmbient.rgb * uLightAmbient.a;
  vec3 final_color = AmbientBRDF(V, N, NdotV, pbr) * radiance_ambient;
  // ambient occlusion
  if (uEnableHbao == 1) final_color *= texture(texHbao, vTexCoords).r;
  if (uEnableGtao == 1) {
    uint packed_value = texture(texGtao, vTexCoords).r;
    final_color *= unpackUnorm4x8(packed_value).a;
  }

  float NdotL = dot(N, L);
  if (NdotL > 0.0) {
    uint cascade = GetCascade(linear_depth);
    float shadow = DirShadowPCF(cascade, vec4(world_pos, 1.0), NdotL);
    if (shadow > 0.0) {
      vec3 radiance = uLightDiffuse.rgb * uLightDiffuse.a * NdotL * shadow;
      final_color += BRDF(V, L, N, NdotV, NdotL, pbr) * radiance;
    }
  }

  final_color += ProcessPointLights(linear_depth, world_pos, V, N, NdotV, pbr);

  fFragColor = vec4(final_color, 1.0);
}