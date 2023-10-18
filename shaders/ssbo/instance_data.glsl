// uvec4(matrix_index, material_index, animation_index, instance/shadow_index)
layout(std430, binding = BIND_ATTRIBUTE_INSTANCE_DATA) INSTANCE_DATA
    restrict buffer InstanceData {
  uvec4 sInstanceData[];
};
