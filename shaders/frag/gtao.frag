#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// OpenGL adaptation of https://github.com/GameTechDev/XeGTAO
// coordinate system remains original (DirectX)

// in
layout(location = 0) in vec2 vTexCoords;

// out
layout(location = 0) out uint fNormalsVisibility;
layout(location = 1) out float fEdges;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/engine.glsl"
#include "shaders/ubo/gtao.glsl"

// textures
layout(binding = 0) uniform sampler2D texLinearDepth;
layout(binding = 1) uniform usampler2D texHilbertIndex;

#define PI 3.1415926535897932384626433832795
#define PI_HALF 1.5707963267948966192313216916398
// for packing in UNORM
// (because raw, pre-denoised occlusion term can overshoot 1
// but will later average out to 1)
#define OCCLUSION_TERM_SCALE 1.5

vec3 ComputeViewspacePosition(vec2 uv, float linear_depth) {
  return vec3((uv * uNdcToViewMul + uNdcToViewAdd) * linear_depth,
              linear_depth);
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

// packing/unpacking for edges
// 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother
// transitions!
float PackEdges(vec4 edges) {
  edges = roundEven(clamp(edges, 0.0, 1.0) * 2.9);
  return dot(edges, vec4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
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

// http://h14s.p5r.org/2012/09/0x5f3759df.html, [Drobot2014a] Low Level
// Optimizations for GCN,
// https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf
// slide 63
float FastSqrt(float x) {
  return uintBitsToFloat(0x1fbd1df5 + (floatBitsToUint(x) >> 1));
}

// input [-1, 1] and output [0, PI], from
// https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
float FastACos(float in_x) {
  const float kPi = 3.141593;
  const float kHalfPi = 1.570796;
  float x = abs(in_x);
  float res = -0.156583 * x + kHalfPi;
  res *= FastSqrt(1.0 - x);
  return (in_x >= 0) ? res : kPi - res;
}

// "Efficiently building a matrix to rotate one vector to another"
// http://cs.brown.edu/research/pubs/pdfs/1999/Moller-1999-EBA.pdf /
// https://dl.acm.org/doi/10.1080/10867651.1999.10487509 (using
// https://github.com/assimp/assimp/blob/master/include/assimp/matrix3x3.inl#L275
// as a code reference as it seems to be best)
mat3 RotFromToMatrix(vec3 from, vec3 to) {
  const float e = dot(from, to);
  //(e < 0)? -e:e;
  const float f = abs(e);

  // WARNING: This has not been tested/worked through, especially not for 16bit
  // floats; seems to work in our special use case (from is always {0, 0, -1})
  // but wouldn't use it in general
  if (f > float(1.0 - 0.0003)) return mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);

  const vec3 v = cross(from, to);
  /* ... use this hand optimized version (9 mults less) */
  /* optimization by Gottfried Chen */
  const float h = (1.0) / (1.0 + e);
  const float hvx = h * v.x;
  const float hvz = h * v.z;
  const float hvxy = hvx * v.y;
  const float hvxz = hvx * v.z;
  const float hvyz = hvz * v.y;

  mat3 mtx;
  mtx[0][0] = e + hvx * v.x;
  mtx[0][1] = hvxy - v.z;
  mtx[0][2] = hvxz + v.y;

  mtx[1][0] = hvxy + v.z;
  mtx[1][1] = e + h * v.y * v.y;
  mtx[1][2] = hvyz - v.x;

  mtx[2][0] = hvxz - v.y;
  mtx[2][1] = hvyz + v.x;
  mtx[2][2] = e + hvz * v.z;

  return mtx;
}

vec2 SpatialNoise() {
  ivec2 pixel_coords = ivec2(gl_FragCoord.xy);
  uint index = texelFetch(texHilbertIndex, pixel_coords % 64, 0).x;
  // R2 sequence - see
  // http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
  return vec2(fract(0.5 + float(index) * vec2(0.75487766624669276005,
                                              0.5698402909980532659114)));
}

void main() {
  // uv from vertex shader or gl_FragCoord.xy point to the center of pixel
  // remove half-texel and get pure pixel_coords (point to corner)
  vec2 uv_gather = floor(gl_FragCoord.xy) * uInvScreenSize;

  // OpenGL UV is bottom-left to upper-right
  // textureGather() returns:
  // vec4([0, 1], [1, 1], [1, 0], [0, 0]) or
  // vec4(top-left, top-right, bottom-right, bottom-left) in CW order
  vec4 bottom_left = textureGather(texLinearDepth, uv_gather, 0);
  vec4 upper_right =
      textureGatherOffset(texLinearDepth, uv_gather, ivec2(1, 1), 0);

  float center_z = bottom_left.y;

  const float left_z = bottom_left.x;
  const float right_z = upper_right.z;
  const float top_z = bottom_left.z;
  const float bottom_z = upper_right.x;

  vec4 edges = CalculateEdges(center_z, left_z, right_z, top_z, bottom_z);

  // Generating screen space normals in-place is faster than generating normals
  // in a separate pass but requires use of 32bit depth buffer (16bit works but
  // visibly degrades quality) which in turn slows everything down. So to reduce
  // complexity and allow for screen space normal reuse by other effects, we've
  // pulled it out into a separate pass. However, we leave this code in, in case
  // anyone has a use-case where it fits better.
  vec3 center = ComputeViewspacePosition(vTexCoords, center_z);
  vec3 left = ComputeViewspacePosition(
      vTexCoords + vec2(-uInvScreenSize.x, 0.0), left_z);
  vec3 right = ComputeViewspacePosition(
      vTexCoords + vec2(uInvScreenSize.x, 0.0), right_z);
  // DirectX UV inverted by y (bottom = top)
  vec3 top = ComputeViewspacePosition(vTexCoords + vec2(0.0, -uInvScreenSize.y),
                                      top_z);
  vec3 bottom = ComputeViewspacePosition(
      vTexCoords + vec2(0.0, uInvScreenSize.y), bottom_z);
  vec3 viewspace_normal =
      CalculateNormal(edges, center, left, right, top, bottom);

  // Move center pixel slightly towards camera to avoid imprecision artifacts
  // due to depth buffer imprecision; offset depends on depth texture format
  // used
  // this is good for FP32 depth buffer
  center_z *= 0.99999;

  const vec3 view_pos = ComputeViewspacePosition(vTexCoords, center_z);
  const vec3 view_dir = normalize(-view_pos);

  // constants
  const float effect_radius = uEffectRadius * uRadiusMultiplier;
  const float falloff_range = uEffectFalloffRange * effect_radius;
  const float falloff_from = effect_radius * (1.0 - uEffectFalloffRange);

  // fadeout precompute optimisation
  const float falloff_mul = -1.0 / (falloff_range);
  const float falloff_add = falloff_from / (falloff_range) + 1.0;

  float visibility = 0.0;
  vec3 bent_normal = vec3(0.0);

  // see "Algorithm 1" in
  // https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
  {
    vec2 local_noise = SpatialNoise();
    const float noise_slice = local_noise.x;
    const float noise_sample = local_noise.y;

    // quality settings / tweaks / hacks
    // if the offset is under approx pixel size
    // (pixel_too_close_threshold), push it out to the minimum distance
    const float pixel_too_close_threshold = 1.3;

    // approx viewspace pixel size at pixCoord
    const vec2 pixel_dir_rb_viewspace_size_at_center_z =
        vec2(center_z) * uNdcToViewMulPixelSize;

    float screenspace_radius =
        effect_radius / pixel_dir_rb_viewspace_size_at_center_z.x;

    // fade out for small screen radii
    visibility += clamp(((10.0 - screenspace_radius) / 100.0), 0.0, 1.0) * 0.5;

    // this is the min distance to start sampling from to avoid sampling from
    // the center pixel (no useful data obtained from sampling center pixel)
    const float min_s = pixel_too_close_threshold / screenspace_radius;

    for (float slice = 0; slice < uSliceCount; slice++) {
      float slice_k = (slice + noise_slice) / uSliceCount;
      // lines 5, 6 from the paper
      float phi = slice_k * PI;
      float cos_phi = cos(phi);
      float sin_phi = sin(phi);
      // vec2 on omega causes issues with big radii
      vec2 omega = vec2(cos_phi, -sin_phi);

      // convert to screen units (pixels) for later use
      omega *= screenspace_radius;

      // line 8 from the paper
      const vec3 direction_vec = vec3(cos_phi, sin_phi, 0.0);

      // line 9 from the paper
      const vec3 ortho_direction_vec =
          direction_vec - (dot(direction_vec, view_dir) * view_dir);

      // line 10 from the paper
      // axis_vec is orthogonal to direction_vec and view_dir, used to define
      // projected_normal
      const vec3 axis_vec = normalize(cross(ortho_direction_vec, view_dir));

      // alternative line 9 from the paper
      // vec3 ortho_direction_vec = cross( view_dir, axis_vec );

      // line 11 from the paper
      vec3 projected_normal_vec =
          viewspace_normal - axis_vec * dot(viewspace_normal, axis_vec);

      // line 13 from the paper
      float sign_norm = sign(dot(ortho_direction_vec, projected_normal_vec));

      // line 14 from the paper
      float projected_normal_vec_length = length(projected_normal_vec);
      float cos_norm = clamp(
          dot(projected_normal_vec, view_dir) / projected_normal_vec_length,
          0.0, 1.0);

      // line 15 from the paper
      float n = sign_norm * FastACos(cos_norm);

      // this is a lower weight target; not using -1 as in the original paper
      // because it is under horizon, so a 'weight' has different meaning based
      // on the normal
      const float low_horizon_cos0 = cos(n + PI_HALF);
      const float low_horizon_cos1 = cos(n - PI_HALF);

      // lines 17, 18 from the paper, manually unrolled the 'side' loop
      float horizon_cos0 = low_horizon_cos0;  //-1;
      float horizon_cos1 = low_horizon_cos1;  //-1;

      for (float stepp = 0; stepp < uStepsPerSlice; stepp++) {
        // R1 sequence
        // (http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/)
        const float step_base_noise =
            float(slice + stepp * uStepsPerSlice) *
            0.6180339887498948482;  // <- this should unroll
        float step_noise = fract(noise_sample + step_base_noise);

        // approx line 20 from the paper, with added noise
        float s = (stepp + step_noise) / (uStepsPerSlice);  // + (vec2)1e-6f);

        // additional distribution modifier
        s = pow(s, uSampleDistributionPower);

        // avoid sampling center pixel
        s += min_s;

        // approx lines 21-22 from the paper, unrolled
        vec2 sample_offset = s * omega;

        float sample_offset_length = length(sample_offset);

        // Snap to pixel center (more correct direction math, avoids artifacts
        // due to sampling pos not matching depth texel center - messes up slope
        // - but adds other artifacts due to them being pushed off the slice).
        // Also use full precision for high res cases.
        sample_offset = roundEven(sample_offset) * uInvScreenSize;

        vec2 sample_screen_pos0 = vTexCoords + sample_offset;
        float SZ0 = texture(texLinearDepth, sample_screen_pos0).r;
        vec3 sample_pos0 = ComputeViewspacePosition(sample_screen_pos0, SZ0);

        vec2 sample_screen_pos1 = vTexCoords - sample_offset;
        float SZ1 = texture(texLinearDepth, sample_screen_pos1).r;
        vec3 sample_pos1 = ComputeViewspacePosition(sample_screen_pos1, SZ1);

        vec3 sample_delta0 = (sample_pos0 - vec3(view_pos));
        vec3 sample_delta1 = (sample_pos1 - vec3(view_pos));

        float sample_dist0 = length(sample_delta0);
        float sample_dist1 = length(sample_delta1);

        // approx lines 23, 24 from the paper, unrolled
        vec3 sample_horizon_vec0 = vec3(sample_delta0 / sample_dist0);
        vec3 sample_horizon_vec1 = vec3(sample_delta1 / sample_dist1);

        // any sample out of radius should be discarded - also use fallof range
        // for smooth transitions; this is a modified idea from "4.3
        // Implementation details, Bounding the sampling area"

        // this is our own thickness heuristic that relies on sooner discarding
        // samples behind the center
        float falloff_base0 =
            length(vec3(sample_delta0.x, sample_delta0.y,
                        sample_delta0.z * (1.0 + uThinOccluderCompensation)));
        float falloff_base1 =
            length(vec3(sample_delta1.x, sample_delta1.y,
                        sample_delta1.z * (1.0 + uThinOccluderCompensation)));

        float weight0 =
            clamp(falloff_base0 * falloff_mul + falloff_add, 0.0, 1.0);
        float weight1 =
            clamp(falloff_base1 * falloff_mul + falloff_add, 0.0, 1.0);

        // sample horizon cos
        float shc0 = dot(sample_horizon_vec0, view_dir);
        float shc1 = dot(sample_horizon_vec1, view_dir);

        // discard unwanted samples
        // this would be more correct but too expensive:
        // cos(mix( acos(low_horizon_cos0), acos(shc0), weight0 ));
        shc0 = mix(low_horizon_cos0, shc0, weight0);
        // this would be more correct but too expensive:
        // cos(mix( acos(low_horizon_cos1), acos(shc1), weight1 ));
        shc1 = mix(low_horizon_cos1, shc1, weight1);

        // thickness heuristic - see "4.3 Implementation details, Height-field
        // assumption considerations"
        // this is a version where thicknessHeuristic is completely disabled
        horizon_cos0 = max(horizon_cos0, shc0);
        horizon_cos1 = max(horizon_cos1, shc1);
      }

      // I can't figure out the slight overdarkening on high slopes, so I'm
      // adding this fudge - in the training set, 0.05 is close (PSNR 21.34) to
      // disabled (PSNR 21.45)
      projected_normal_vec_length = mix(projected_normal_vec_length, 1.0, 0.05);

      // line ~27, unrolled
      float h0 = -FastACos(horizon_cos1);
      float h1 = FastACos(horizon_cos0);

      float iarc0 = (cos_norm + 2.0 * h0 * sin(n) - cos(2.0 * h0 - n)) / 4.0;
      float iarc1 = (cos_norm + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) / 4.0;
      float local_visibility = projected_normal_vec_length * (iarc0 + iarc1);
      visibility += local_visibility;

      // see "Algorithm 2 Extension that computes bent normals b."
      float t0 =
          (6 * sin(h0 - n) - sin(3 * h0 - n) + 6 * sin(h1 - n) -
           sin(3 * h1 - n) + 16 * sin(n) - 3 * (sin(h0 + n) + sin(h1 + n))) /
          12;
      float t1 = (-cos(3 * h0 - n) - cos(3 * h1 - n) + 8 * cos(n) -
                  3 * (cos(h0 + n) + cos(h1 + n))) /
                 12;

      vec3 local_bent_normal =
          vec3(direction_vec.x * t0, direction_vec.y * t0, -t1);
      mat3 rot_mat = RotFromToMatrix(vec3(0.0, 0.0, -1.0), view_dir);
      local_bent_normal = rot_mat * local_bent_normal;
      local_bent_normal *= projected_normal_vec_length;
      bent_normal += local_bent_normal;
    }
    visibility /= uSliceCount;
    visibility = pow(visibility, uFinalValuePower);
    // disallow total occlusion (which wouldn't make any
    // sense anyhow since pixel is visible but also helps
    // with packing bent normals)
    visibility = max(0.03, visibility);
    bent_normal = normalize(bent_normal);
  }

  visibility = clamp(visibility / float(OCCLUSION_TERM_SCALE), 0.0, 1.0);
  fNormalsVisibility = packUnorm4x8(vec4(bent_normal * 0.5 + 0.5, visibility));
  fEdges = PackEdges(edges);
}
