struct FxPreset {
  mat4 emitter_basis;
  vec4 acceleration;
  vec4 direction_constraints;
  vec2 start_position_min_max;
  vec2 start_velocity_min_max;
  //
  float particle_lifetime;
  float cone_angle_rad;
  uint num_of_particles;
  uint buffer_offset;
};

// read and write buffer
// no restrict (maybe should)
// each thread take one variable, no coherence
layout(std430, binding = BIND_FX_PRESETS) buffer StorageFxPresets {
  FxPreset sFxPresets[MAX_FX_PRESETS];
  vec4 sPositions[MAX_FXS_BUFFER_SIZE];
  vec4 sVelocities[MAX_FXS_BUFFER_SIZE];
  float sAges[MAX_FXS_BUFFER_SIZE];
};
