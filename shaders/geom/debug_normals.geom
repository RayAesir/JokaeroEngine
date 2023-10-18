#version 460 core

// geometry
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

// in
in VertexData {
  vec4 clip_pos;
  vec4 normal_pos;
}
gs_in[];

void DrawLine(int index) {
  gl_Position = gs_in[index].clip_pos;
  EmitVertex();
  gl_Position = gs_in[index].normal_pos;
  EmitVertex();
  EndPrimitive();
}

void main() {
  DrawLine(0);
  DrawLine(1);
  DrawLine(2);
}
