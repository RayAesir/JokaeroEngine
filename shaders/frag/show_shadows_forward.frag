#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

#ifndef ALPHA
layout(early_fragment_tests) in;
#endif

// in
layout(location = 0) in VertexData {
  vec4 frag_pos;
  mat3 tangent_to_world;
  vec2 tex_coords;
  flat uint material_index;
}
fs_in;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/lighting.glsl"
#include "shaders/ubo/shadows.glsl"

// ssbo
#define STORAGE_MATERIALS readonly
#define STORAGE_POINT_LIGHTS readonly
#define STORAGE_FRUSTUM_POINT_LIGHTS readonly
#define STORAGE_CLUSTER_LIGHTS readonly
#include "shaders/ssbo/cluster_lights.glsl"
#include "shaders/ssbo/frustum_point_lights.glsl"
#include "shaders/ssbo/materials.glsl"
#include "shaders/ssbo/point_lights.glsl"

// pipeline textures
layout(binding = 0) uniform sampler2DArrayShadow texShadowsPcf;
layout(binding = 1) uniform samplerCubeArrayShadow texPointShadowsPcf;
layout(binding = 2) uniform sampler3D texRandomAngles;

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
float ProcessPointLights(float linear_depth, vec3 world_pos, vec3 N) {
  float final_shadow = 0.0;

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
          final_shadow += shadow;
        }
        //
      }
    }
  }

  return final_shadow;
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

void main() {
  Material mat = sMaterials[fs_in.material_index];
  vec4 albedo = GetDiffuse(mat, fs_in.tex_coords);

#ifdef ALPHA
  if (albedo.a <= mat.alpha_threshold) discard;
#endif

  vec3 normal = GetNormals(mat, fs_in.tex_coords);
  vec3 N = normalize(fs_in.tangent_to_world * normal);
  float linear_depth = uProj[3][2] / (uProj[2][2] + gl_FragCoord.z);

  vec3 L = uLightDir.xyz;

  float shadow = 0.0;
  float NdotL = dot(N, L);
  if (NdotL > 0.0) {
    uint cascade = GetCascade(linear_depth);
    shadow = DirShadowPCF(cascade, fs_in.frag_pos, NdotL);
  }

  shadow += ProcessPointLights(linear_depth, fs_in.frag_pos.xyz, N);

  fFragColor = vec4(vec3(shadow), 1.0);
}