#version 460 core

// in
layout(location = 0) in vec4 vColor;

// out
layout(location = 0) out vec4 fFragColor;

void main() { fFragColor = vColor; }
