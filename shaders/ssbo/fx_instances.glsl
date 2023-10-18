struct FxInstance {
  vec2 particle_size_min_max;
  float particle_lifetime;
  float should_keep_color;
  //
  vec4 color;
  vec4 emitter_world_pos;
  //
  uvec2 tex_handler;
  uvec2 empty_pad0;
};

layout(std430, binding = BIND_FX_INSTANCES) readonly
    restrict buffer StorageFxInstances {
  FxInstance sParticles[];
};
