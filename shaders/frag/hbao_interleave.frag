#version 460 core

// based on
// https://github.com/nvpro-samples/gl_ssao

// out
layout(location = 0) out vec4 fFragColor;

// textures
layout(binding = 0) uniform sampler2DArray uResultArray;

void main() {
  // converting to integers remove the 0.5 part
  ivec2 full_res = ivec2(gl_FragCoord.xy);
  // [0, 3], a fancy way to write: full_res % 4 or mod(full_res, 4)
  ivec2 offset = full_res & 3;
  // [0, 15]
  int slice_id = offset.y * 4 + offset.x;
  // bitshift, divide by 4
  ivec2 quater_res = full_res >> 2;

  fFragColor =
      vec4(texelFetch(uResultArray, ivec3(quater_res, slice_id), 0).xy, 0, 0);
}