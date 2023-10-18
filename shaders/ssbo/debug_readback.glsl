// used to debug shaders:
// sInt[0] = value;
// memoryBarrierBuffer();
// barrier();
layout(std430, binding = BIND_DEBUG_READBACK) coherent writeonly buffer
    StorageDebugReadback {
  int sInt[DEBUG_READBACK_SIZE];
  uint sUint[DEBUG_READBACK_SIZE];
  float sFloat[DEBUG_READBACK_SIZE];
  vec4 sVec4[DEBUG_READBACK_SIZE];
};
