struct Instance {
  uint object_id;
  uint props;
  uint packed_cmd_index;
  uint matrix_index;
  uint material_index;
  uint animation_index;
  uint box_index;
};

layout(std430, binding = BIND_INSTANCES) STORAGE_INSTANCES
    restrict buffer StorageInstances {
  Instance sInstances[];
};
