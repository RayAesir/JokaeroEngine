#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

// header
#include "shaders/header.glsl"

// out
layout(location = 0) out float vTransparency;
layout(location = 1) out vec2 vTexCoords;
layout(location = 2) out int vDrawID;

// ubo
#include "shaders/ubo/camera.glsl"

// ssbo
#define STORAGE_FX_PRESETS
#define STORAGE_FX_INSTANCES readonly
#include "shaders/ssbo/fx_instances.glsl"
#include "shaders/ssbo/fx_presets.glsl"

// offsets to the position in camera coordinates for each vertex of the
// particle's quad
const vec3 kOffsets[] =
    vec3[](vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3(0.5, 0.5, 0),
           vec3(-0.5, -0.5, 0), vec3(0.5, 0.5, 0), vec3(-0.5, 0.5, 0));

// texture coordinates for each vertex of the particle's quad
const vec2 kTexCoords[] = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0),
                                 vec2(1, 1), vec2(0, 1));

void main() {
  FxInstance instance = sParticles[gl_DrawID];
  vec2 particle_size_min_max = instance.particle_size_min_max;
  float particle_lifetime = instance.particle_lifetime;
  vec4 emitter_world_pos = instance.emitter_world_pos;

  // base_instance as buffer_offset
  int idx = gl_BaseInstance + gl_InstanceID;

  vTransparency = 0.0;
  vec3 pos_view_space = vec3(0.0);

  if (sAges[idx] >= 0.0) {
    float age_pct = sAges[idx] / particle_lifetime;
    vTransparency = clamp(1.0 - age_pct, 0.0, 1.0);
    vec3 quad_pct =
        kOffsets[gl_VertexID] *
        mix(particle_size_min_max.x, particle_size_min_max.y, age_pct);
    vec4 quad_world_pos = sPositions[idx] + emitter_world_pos;
    pos_view_space = (uView * quad_world_pos).xyz + quad_pct;
  }

  vTexCoords = kTexCoords[gl_VertexID];
  vDrawID = gl_DrawID;
  gl_Position = uProj * vec4(pos_view_space, 1.0);
}