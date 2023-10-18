#version 460 core

// out
layout(location = 0) out vec2 vTexCoords;

// constants
// counter-clockwise order
const vec2 kTriangle[3] =
    vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

// https://wallisc.github.io/rendering/2021/04/18/Fullscreen-Pass.html
void main() {
  // multiple screens support
  int index = int(mod(gl_VertexID, 3));
  vec4 position = vec4(kTriangle[index], 0.0, 1.0);
  gl_Position = position;
  // [0, 0], [2, 0], [0, 2]
  vTexCoords = 0.5 * position.xy + 0.5;
}
