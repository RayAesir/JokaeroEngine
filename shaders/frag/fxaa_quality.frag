#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out vec4 fFragColor;

// ubo
#include "shaders/ubo/engine.glsl"

// textures
layout(binding = 0) uniform sampler2D texLdrLuma;

// settings for FXAA (high quality)
#define EDGE_THRESHOLD_MIN (1.0 / 16.0)
#define EDGE_THRESHOLD_MAX (1.0 / 8.0)
#define QUALITY(q) \
  ((q) < 5 ? 1.0 : ((q) > 5 ? ((q) < 10 ? 2.0 : ((q) < 11 ? 4.0 : 8.0)) : 1.5))
#define ITERATIONS 12
#define SUBPIXEL_QUALITY 0.75

// performs FXAA post-process anti-aliasing as described in the Nvidia FXAA
// white paper and the associated shader code
void main() {
  vec3 color_center = texture(texLdrLuma, vTexCoords).rgb;

  // luma at the current fragment
  float L = texture(texLdrLuma, vTexCoords).a;

  // luma at the four direct neighbours of the current fragment
  float L_down = textureOffset(texLdrLuma, vTexCoords, ivec2(0, -1)).a;
  float L_up = textureOffset(texLdrLuma, vTexCoords, ivec2(0, 1)).a;
  float L_left = textureOffset(texLdrLuma, vTexCoords, ivec2(-1, 0)).a;
  float L_right = textureOffset(texLdrLuma, vTexCoords, ivec2(1, 0)).a;

  // find the maximum and minimum luma around the current fragment
  float L_min = min(L, min(min(L_down, L_up), min(L_left, L_right)));
  float L_max = max(L, max(max(L_down, L_up), max(L_left, L_right)));

  // compute the delta
  float L_range = L_max - L_min;

  // if the luma variation is lower that a threshold (or if we are in a really
  // dark area), we are not on an edge, don't perform any AA
  if (L_range < max(EDGE_THRESHOLD_MIN, L_max * EDGE_THRESHOLD_MAX)) {
    fFragColor = vec4(color_center, 1.0);
    return;
  }

  // query the 4 remaining corners lumas
  float L_down_left = textureOffset(texLdrLuma, vTexCoords, ivec2(-1, -1)).a;
  float L_up_right = textureOffset(texLdrLuma, vTexCoords, ivec2(1, 1)).a;
  float L_up_left = textureOffset(texLdrLuma, vTexCoords, ivec2(-1, 1)).a;
  float L_down_right = textureOffset(texLdrLuma, vTexCoords, ivec2(1, -1)).a;

  // combine the four edges lumas (using intermediary variables for future
  // computations with the same values)
  float L_down_up = L_down + L_up;
  float L_left_right = L_left + L_right;

  // same for corners
  float L_left_corners = L_down_left + L_up_left;
  float L_down_corners = L_down_left + L_down_right;
  float L_right_corners = L_down_right + L_up_right;
  float L_up_corners = L_up_right + L_up_left;

  // compute an estimation of the gradient along the horizontal and vertical
  // axis
  float edge_horizontal = abs(-2.0 * L_left + L_left_corners) +
                          abs(-2.0 * L + L_down_up) * 2.0 +
                          abs(-2.0 * L_right + L_right_corners);
  float edge_vertical = abs(-2.0 * L_up + L_up_corners) +
                        abs(-2.0 * L + L_left_right) * 2.0 +
                        abs(-2.0 * L_down + L_down_corners);

  // is the local edge horizontal or vertical?
  bool is_horizontal = (edge_horizontal >= edge_vertical);

  // choose the step size (one pixel) accordingly
  float step_length = is_horizontal ? uInvScreenSize.y : uInvScreenSize.x;

  // select the two neighboring texels lumas in the opposite direction to the
  // local edge
  float luma1 = is_horizontal ? L_down : L_left;
  float luma2 = is_horizontal ? L_up : L_right;
  // compute gradients in this direction
  float gradient1 = luma1 - L;
  float gradient2 = luma2 - L;

  // which direction is the steepest?
  bool is1_steepest = abs(gradient1) >= abs(gradient2);

  // gradient in the corresponding direction, normalized
  float gradient_scaled = 0.25 * max(abs(gradient1), abs(gradient2));

  // average luma in the correct direction
  float luma_local_average = 0.0;
  if (is1_steepest) {
    // switch the direction
    step_length = -step_length;
    luma_local_average = 0.5 * (luma1 + L);
  } else {
    luma_local_average = 0.5 * (luma2 + L);
  }

  // shift UV in the correct direction by half a pixel
  vec2 current_uv = vTexCoords;
  if (is_horizontal) {
    current_uv.y += step_length * 0.5;
  } else {
    current_uv.x += step_length * 0.5;
  }

  // compute offset (for each iteration step) in the right direction
  vec2 offset =
      is_horizontal ? vec2(uInvScreenSize.x, 0.0) : vec2(0.0, uInvScreenSize.y);
  // compute UVs to explore on each side of the edge, orthogonally
  // the QUALITY allows us to step faster
  vec2 uv1 = current_uv - offset * QUALITY(0);
  vec2 uv2 = current_uv + offset * QUALITY(0);

  // read the lumas at both current extremities of the exploration segment, and
  // compute the delta wrt to the local average luma
  float L_end1 = texture(texLdrLuma, uv1).a;
  float L_end2 = texture(texLdrLuma, uv2).a;
  L_end1 -= luma_local_average;
  L_end2 -= luma_local_average;

  // if the luma deltas at the current extremities is larger than the local
  // gradient, we have reached the side of the edge
  bool reached1 = abs(L_end1) >= gradient_scaled;
  bool reached2 = abs(L_end2) >= gradient_scaled;
  bool reached_both = reached1 && reached2;

  // if the side is not reached, we continue to explore in this direction
  if (!reached1) {
    uv1 -= offset * QUALITY(1);
  }
  if (!reached2) {
    uv2 += offset * QUALITY(1);
  }

  // if both sides have not been reached, continue to explore
  if (!reached_both) {
    for (int i = 2; i < ITERATIONS; i++) {
      // if needed, read luma in 1st direction, compute delta
      if (!reached1) {
        L_end1 = texture(texLdrLuma, uv1).a;
        L_end1 = L_end1 - luma_local_average;
      }
      // if needed, read luma in opposite direction, compute delta
      if (!reached2) {
        L_end2 = texture(texLdrLuma, uv2).a;
        L_end2 = L_end2 - luma_local_average;
      }
      // if the luma deltas at the current extremities is larger than the local
      // gradient, we have reached the side of the edge
      reached1 = abs(L_end1) >= gradient_scaled;
      reached2 = abs(L_end2) >= gradient_scaled;
      reached_both = reached1 && reached2;

      // if the side is not reached, we continue to explore in this direction,
      // with a variable quality
      if (!reached1) {
        uv1 -= offset * QUALITY(i);
      }
      if (!reached2) {
        uv2 += offset * QUALITY(i);
      }

      // if both sides have been reached, stop the exploration
      if (reached_both) {
        break;
      }
    }
  }

  // compute the distances to each side edge of the edge (!)
  float distance1 =
      is_horizontal ? (vTexCoords.x - uv1.x) : (vTexCoords.y - uv1.y);
  float distance2 =
      is_horizontal ? (uv2.x - vTexCoords.x) : (uv2.y - vTexCoords.y);

  // in which direction is the side of the edge closer?
  bool is_direction1 = distance1 < distance2;
  float distance_final = min(distance1, distance2);

  // thickness of the edge
  float edge_thickness = (distance1 + distance2);

  // is the luma at center smaller than the local average?
  bool is_luma_center_smaller = L < luma_local_average;

  // if the luma at center is smaller than at its neighbour, the delta luma at
  // each end should be positive (same variation)
  bool correct_variation1 = (L_end1 < 0.0) != is_luma_center_smaller;
  bool correct_variation2 = (L_end2 < 0.0) != is_luma_center_smaller;

  // only keep the result in the direction of the closer side of the edge
  bool correct_variation =
      is_direction1 ? correct_variation1 : correct_variation2;

  // UV offset: read in the direction of the closest side of the edge
  float pixel_offset = -distance_final / edge_thickness + 0.5;

  // if the luma variation is incorrect, do not offset
  float final_offset = correct_variation ? pixel_offset : 0.0;

  // sub-pixel shifting
  // full weighted average of the luma over the 3x3 neighborhood
  float luma_average = (1.0 / 12.0) * (2.0 * (L_down_up + L_left_right) +
                                       L_left_corners + L_right_corners);
  // ratio of the delta between the global average and the center luma, over the
  // luma range in the 3x3 neighborhood
  float sub_pixel_offset1 = clamp(abs(luma_average - L) / L_range, 0.0, 1.0);
  float sub_pixel_offset2 =
      (-2.0 * sub_pixel_offset1 + 3.0) * sub_pixel_offset1 * sub_pixel_offset1;
  // compute a sub-pixel offset based on this delta
  float sub_pixel_offset_final =
      sub_pixel_offset2 * sub_pixel_offset2 * SUBPIXEL_QUALITY;

  // pick the biggest of the two offsets
  final_offset = max(final_offset, sub_pixel_offset_final);

  // compute the final UV coordinates
  vec2 final_uv = vTexCoords;
  if (is_horizontal) {
    final_uv.y += final_offset * step_length;
  } else {
    final_uv.x += final_offset * step_length;
  }

  // read the color at the new UV coordinates, and use it
  vec3 finalColor = texture(texLdrLuma, final_uv).rgb;
  fFragColor = vec4(finalColor, 1.0);
}