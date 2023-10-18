#version 460 core

// geometry
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// in
layout(location = 0) in vec2 vTexCoords[];

// out
layout(location = 0) out vec2 gTexCoords;

void main() {
  for (int i = 0; i < 3; ++i) {
    gTexCoords = vTexCoords[i];
    gl_Layer = gl_PrimitiveIDIn;
    gl_PrimitiveID = gl_PrimitiveIDIn;
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }
}
