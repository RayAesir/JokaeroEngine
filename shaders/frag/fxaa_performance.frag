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

// constants
const vec3 kLuma = vec3(0.299, 0.587, 0.114);
const float kLumaThreshold = 0.5;
const float kMulReduce = 1.0 / 16.0;
const float kMinReduce = 1.0 / 128.0;
const float kMaxSpan = 8.0;

// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
// http://iryoku.com/aacourse/downloads/09-FXAA-3.11-in-15-Slides.pdf
// http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
void main(void) {
  vec3 rgb_m = texture(texLdrLuma, vTexCoords).rgb;

  // sampling neighbour texels
  float luma_nw = textureOffset(texLdrLuma, vTexCoords, ivec2(-1, 1)).a;
  float luma_ne = textureOffset(texLdrLuma, vTexCoords, ivec2(1, 1)).a;
  float luma_sw = textureOffset(texLdrLuma, vTexCoords, ivec2(-1, -1)).a;
  float luma_se = textureOffset(texLdrLuma, vTexCoords, ivec2(1, -1)).a;
  float luma_m = texture(texLdrLuma, vTexCoords).a;

  // gather minimum and maximum luma
  float luma_min =
      min(luma_m, min(min(luma_nw, luma_ne), min(luma_sw, luma_se)));
  float luma_max =
      max(luma_m, max(max(luma_nw, luma_ne), max(luma_sw, luma_se)));

  // if contrast is lower than a maximum threshold
  if (luma_max - luma_min <= luma_max * kLumaThreshold) {
    // do no AA and return
    fFragColor = vec4(rgb_m, 1.0);
    return;
  }

  // sampling is done along the gradient
  vec2 sampling_direction;
  sampling_direction.x = -((luma_nw + luma_ne) - (luma_sw + luma_se));
  sampling_direction.y = ((luma_nw + luma_sw) - (luma_ne + luma_se));

  // sampling step distance depends on the luma: The brighter the sampled
  // texels, the smaller the final sampling step direction
  // this results, that brighter areas are less blurred/more sharper than dark
  // areas
  float sampling_direction_reduce = max(
      (luma_nw + luma_ne + luma_sw + luma_se) * 0.25 * kMulReduce, kMinReduce);

  // factor for norming the sampling direction plus adding the brightness
  // influence
  float min_sampling_direction_factor =
      1.0 / (min(abs(sampling_direction.x), abs(sampling_direction.y)) +
             sampling_direction_reduce);

  // calculate final sampling direction vector by reducing, clamping to a range
  // and finally adapting to the texture size
  sampling_direction = clamp(sampling_direction * min_sampling_direction_factor,
                             vec2(-kMaxSpan), vec2(kMaxSpan)) *
                       uInvScreenSize;

  // inner samples on the tab
  vec3 rgb_sample_neg =
      texture(texLdrLuma, vTexCoords + sampling_direction * (1.0 / 3.0 - 0.5))
          .rgb;
  vec3 rgb_sample_pos =
      texture(texLdrLuma, vTexCoords + sampling_direction * (2.0 / 3.0 - 0.5))
          .rgb;

  vec3 rgb_two_tab = (rgb_sample_pos + rgb_sample_neg) * 0.5;

  // outer samples on the tab
  vec3 rgb_sample_neg_outer =
      texture(texLdrLuma, vTexCoords + sampling_direction * (0.0 / 3.0 - 0.5))
          .rgb;
  vec3 rgb_sample_pos_outer =
      texture(texLdrLuma, vTexCoords + sampling_direction * (3.0 / 3.0 - 0.5))
          .rgb;

  vec3 rgb_four_tab =
      (rgb_sample_pos_outer + rgb_sample_neg_outer) * 0.25 + rgb_two_tab * 0.5;

  // calculate luma for checking against the minimum and maximum value
  float luma_four_tab = dot(rgb_four_tab, kLuma);

  // are outer samples of the tab beyond the edge
  if (luma_four_tab < luma_min || luma_four_tab > luma_max) {
    // yes, so use only two samples
    fFragColor = vec4(rgb_two_tab, 1.0);
  } else {
    // no, so use four samples
    fFragColor = vec4(rgb_four_tab, 1.0);
  }
}