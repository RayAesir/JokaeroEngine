struct StorageCmd {
  uint count;
  uint first_index;
  uint base_vertex;
  uint packed_cmd_index;
};

layout(std430, binding = BIND_INDIRECT_CMD_STORAGE) INDIRECT_CMD_STORAGE
    restrict buffer IndirectCmdStorage {
  StorageCmd sStorageCmd[];
};
