layout(std430, binding = BIND_ANIMATIONS) readonly
    restrict buffer StorageAnimations {
  mat4 sBoneMatrices[];
};

mat4 GetSkinMat(uint animation_index, ivec4 bones_ids, vec4 weights) {
  int offset = int(animation_index);
  mat4 skin = sBoneMatrices[offset + bones_ids[0]] * weights[0];
  skin += sBoneMatrices[offset + bones_ids[1]] * weights[1];
  skin += sBoneMatrices[offset + bones_ids[2]] * weights[2];
  skin += sBoneMatrices[offset + bones_ids[3]] * weights[3];
  return skin;
}
