#version 460

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

// textures
layout(binding = 0) uniform sampler2D texNormalsDepth;

// ubo
#include "shaders/ubo/engine.glsl"

// https://roystan.net/articles/outline-shader/
void main() {
  const vec3 outline_color = uOutlineColorWidth.rgb;
  const float outline_width = uOutlineColorWidth.a;

  float half_width_scale_floor = floor(outline_width);
  float half_width_scale_ceil = ceil(outline_width);

  vec2 pixel_size = 1.0 / textureSize(texNormalsDepth, 0);

  vec4 normal_depth = texture(texNormalsDepth, vTexCoords);

  vec2 bottom_left = vTexCoords - pixel_size * half_width_scale_floor;
  vec2 top_right = vTexCoords + pixel_size * half_width_scale_ceil;
  vec2 bottom_right = vTexCoords + vec2(pixel_size.x * half_width_scale_ceil,
                                        -pixel_size.y * half_width_scale_floor);
  vec2 top_left = vTexCoords + vec2(-pixel_size.x * half_width_scale_floor,
                                    -pixel_size.y * half_width_scale_ceil);

  vec3 normal0 = texture(texNormalsDepth, bottom_left).rgb;
  vec3 normal1 = texture(texNormalsDepth, top_right).rgb;
  vec3 normal2 = texture(texNormalsDepth, bottom_right).rgb;
  vec3 normal3 = texture(texNormalsDepth, top_left).rgb;

  float depth0 = texture(texNormalsDepth, bottom_left).a;
  float depth1 = texture(texNormalsDepth, top_right).a;
  float depth2 = texture(texNormalsDepth, bottom_right).a;
  float depth3 = texture(texNormalsDepth, top_left).a;

  vec3 view_normal = normal_depth.rgb;
  float NdotV = 1.0 - dot(view_normal, -vFrustumRay);

  float n_threshold =
      clamp((NdotV - uDepthNormalThreshold) / (1.0001 - uDepthNormalThreshold),
            0.0, 1.0);
  n_threshold = n_threshold * uDepthNormalThresholdScale + 1.0;

  float d_threshold = uDepthThreshold * normal_depth.a * n_threshold;

  float depth_finite_diff0 = depth1 - depth0;
  float depth_finite_diff1 = depth3 - depth2;

  // edgeDepth is calculated using the Roberts cross operator.
  // The same operation is applied to the normal below.
  // https://en.wikipedia.org/wiki/Roberts_cross
  float edge_depth =
      sqrt(pow(depth_finite_diff0, 2) + pow(depth_finite_diff1, 2)) * 100.0;
  edge_depth = edge_depth > d_threshold ? 1.0 : 0.0;

  vec3 normal_finite_diff0 = normal1 - normal0;
  vec3 normal_finite_diff1 = normal3 - normal2;

  float edge_normal = sqrt(dot(normal_finite_diff0, normal_finite_diff0) +
                           dot(normal_finite_diff1, normal_finite_diff1));
  edge_normal = edge_normal > uNormalThreshold ? 1.0 : 0.0;

  float edge = max(edge_depth, edge_normal);
  vec4 edge_color = vec4(outline_color * edge, 1.0);

  fFragColor = edge_color;
}