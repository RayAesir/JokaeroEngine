struct DrawElementsIndirectCommand {
  uint count;
  uint instance_count;
  uint first_index;
  uint base_vertex;
  uint base_instance;
};

layout(std430, binding = BIND_INDIRECT_CMD_DRAW) INDIRECT_CMD_DRAW
    restrict buffer IndirectCmdDraw {
  DrawElementsIndirectCommand sDrawCmd[];
};
