#version 460 core

// out
layout(location = 0) out vec4 fFragColor;

// uniform
layout(location = 0) uniform vec3 uColor;

void main() { fFragColor = vec4(uColor, 1.0); }