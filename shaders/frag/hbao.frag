#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// based on
// https://github.com/nvpro-samples/gl_ssao

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out vec2 fFragColor;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/engine.glsl"
#include "shaders/ubo/hbao.glsl"

// textures
layout(binding = 0) uniform sampler2DArray texLinearDepthArray;

// functions

// constants
const float kNumSteps = 4;
// texRandom/gJitter initialization depends on this
const float kNumDirections = 8;
const float kPi = 3.14159265;

vec3 ReconstructViewPos(vec2 uv, float linear_depth) {
  return vec3((uv * uProjInfo.xy + uProjInfo.zw) * linear_depth, -linear_depth);
}

vec4 CalculateEdges(float center_z, float left_z, float right_z, float top_z,
                    float bottom_z) {
  vec4 edges = vec4(left_z, right_z, top_z, bottom_z) - center_z;

  float slope_x = (edges.y - edges.x) * 0.5;
  float slope_y = (edges.w - edges.z) * 0.5;
  vec4 edges_slope_adjusted =
      edges + vec4(slope_x, -slope_x, slope_y, -slope_y);
  edges = min(abs(edges), abs(edges_slope_adjusted));
  return vec4(clamp((1.25 - edges / (center_z * 0.011)), 0.0, 1.0));
}

vec3 CalculateNormal(vec4 edges, vec3 center, vec3 left, vec3 right, vec3 top,
                     vec3 bottom) {
  vec4 accepted_normals = clamp(vec4(edges.x * edges.z, edges.z * edges.y,
                                     edges.y * edges.w, edges.w * edges.x) +
                                    0.01,
                                0.0, 1.0);

  left = normalize(left - center);
  right = normalize(right - center);
  top = normalize(top - center);
  bottom = normalize(bottom - center);

  vec3 normal = accepted_normals.x * cross(left, top) +
                accepted_normals.y * cross(top, right) +
                accepted_normals.z * cross(right, bottom) +
                accepted_normals.w * cross(bottom, left);
  normal = normalize(normal);

  return normal;
}

vec3 FetchQuarterResViewPos(vec2 uv) {
  vec3 P = vec3(uv, float(gl_PrimitiveID));
  float linear_depth = texture(texLinearDepthArray, P).r;
  return ReconstructViewPos(uv, linear_depth);
}

vec3 FetchQuarterResViewPosDepth(vec2 uv, out float z) {
  vec3 P = vec3(uv, float(gl_PrimitiveID));
  float linear_depth = texture(texLinearDepthArray, P).r;
  z = linear_depth;
  return ReconstructViewPos(uv, linear_depth);
}

vec3 ReconstructNormal(vec2 uv, vec3 center, float center_z) {
  float left_z;
  float right_z;
  float top_z;
  float bottom_z;
  // get view_pos at 1 pixel offsets in each major direction
  // top/bottom is inverted
  vec3 right = FetchQuarterResViewPosDepth(
      uv + vec2(uInvQuarterResolution.x, 0.0), right_z);
  vec3 left = FetchQuarterResViewPosDepth(
      uv + vec2(-uInvQuarterResolution.x, 0.0), left_z);
  vec3 top = FetchQuarterResViewPosDepth(
      uv + vec2(0.0, -uInvQuarterResolution.y), top_z);
  vec3 bottom = FetchQuarterResViewPosDepth(
      uv + vec2(0.0, uInvQuarterResolution.y), bottom_z);
  vec4 edges = CalculateEdges(center_z, left_z, right_z, top_z, bottom_z);
  return CalculateNormal(edges, center, left, right, top, bottom);
}

float Falloff(float dist_sqr) {
  // 1 scalar mad instruction
  return dist_sqr * uNegInvSqrRadius + 1.0;
}

// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
float ComputeAO(vec3 P, vec3 N, vec3 S) {
  vec3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

  // use clamp(x) instead of max(x, 0.0) because that is faster on Kepler
  return clamp(NdotV - uBias, 0.0, 1.0) * clamp(Falloff(VdotV), 0.0, 1.0);
}

vec2 RotateDirection(vec2 dir, vec2 cos_sin) {
  return vec2(dir.x * cos_sin.x - dir.y * cos_sin.y,
              dir.x * cos_sin.y + dir.y * cos_sin.x);
}

float ComputeCoarseAO(vec2 full_res_uv, float radius_pixels, vec4 rand,
                      vec3 view_pos, vec3 view_normal) {
  // deinterleaved
  radius_pixels /= 4.0;

  // divide by kNumSteps+1 so that the farthest samples are not fully attenuated
  float step_size_pixels = radius_pixels / (kNumSteps + 1);
  const float kAlpha = 2.0 * kPi / kNumDirections;
  float ao = 0.0;
  for (float dir_index = 0.0; dir_index < kNumDirections; ++dir_index) {
    float angle = kAlpha * dir_index;

    // compute normalized 2D direction
    vec2 direction = RotateDirection(vec2(cos(angle), sin(angle)), rand.xy);

    // jitter starting sample within the first step
    float ray_pixels = (rand.z * step_size_pixels + 1.0);

    for (float step_index = 0; step_index < kNumSteps; ++step_index) {
      vec2 snapped_uv =
          round(ray_pixels * direction) * uInvQuarterResolution + full_res_uv;
      vec3 S = FetchQuarterResViewPos(snapped_uv);

      ray_pixels += step_size_pixels;

      ao += ComputeAO(view_pos, view_normal, S);
    }
  }

  ao *= uAoMultiplier / (kNumDirections * kNumSteps);
  return clamp(1.0 - ao * 2.0, 0.0, 1.0);
}

void main() {
  vec2 float2_offset = uFloat2Offsets[gl_PrimitiveID].xy;
  vec2 pixel_coords = floor(gl_FragCoord.xy) * 4.0 + float2_offset;
  vec2 uv = pixel_coords * uInvScreenSize;

  // view space, z is negative
  float linear_depth;
  vec3 view_pos = FetchQuarterResViewPosDepth(uv, linear_depth);
  vec3 view_normal = ReconstructNormal(uv, view_pos, linear_depth);

  // no normals (sky), no ambient occlusion
  if (view_normal == vec3(0.0, 0.0, 1.0)) discard;

  // compute projection of disk of radius control.R into
  // screen space projection
  float radius_pixels = uRadiusToScreen / linear_depth;

  // jitter vector from the per-pass constant buffer
  // for the current full-res pixel
  vec4 rand = uJitters[gl_PrimitiveID];

  float ao = ComputeCoarseAO(uv, radius_pixels, rand, view_pos, view_normal);

  // blur use green channel's depth
  fFragColor = vec2(pow(ao, uExponent), linear_depth);
}