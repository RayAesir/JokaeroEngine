#version 460 core

#ifdef GL_GOOGLE_include_directive
#extension GL_GOOGLE_include_directive : enable
#endif

#extension GL_ARB_bindless_texture : enable

// header
#include "shaders/header.glsl"

// in
layout(location = 0) in float vTransparency;
layout(location = 1) in vec2 vTexCoords;
flat layout(location = 2) in int vDrawID;

// out
layout(location = 0) out vec4 fColorAccum;
layout(location = 1) out float fRevealAlpha;

// ubo
#include "shaders/ubo/camera.glsl"
#include "shaders/ubo/engine.glsl"

// ssbo
#define STORAGE_FX_INSTANCES readonly
#include "shaders/ssbo/fx_instances.glsl"

void main() {
  FxInstance instance = sParticles[vDrawID];
  float should_keep_color = instance.should_keep_color;
  vec3 color = instance.color.rgb;

  // grayscale texture with alpha channel
  uvec2 hndParticle = instance.tex_handler;
  sampler2D texParticle = sampler2D(hndParticle);
  vec4 tex = texture(texParticle, vTexCoords);

  vec3 tex_color = color * tex.r;
  float alpha = vTransparency * tex.a;
  vec3 final_color =
      mix(vec3(0.0), tex_color, max(vTransparency, should_keep_color));

  // normalized linear depth [0.0, 1.0]
  float linear_depth = (uProj[3][2] / (uProj[2][2] + gl_FragCoord.z));
  float normalized_depth = linear_depth * uInvDepthRange;
  float tmp = (1.0 - normalized_depth) * alpha;
  float weight = clamp(pow(tmp, uParticlesPow), 0.0001, uParticlesClamp);

  fColorAccum = vec4(final_color * alpha, alpha) * weight;
  fRevealAlpha = alpha;
}
