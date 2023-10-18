#pragma once

// deps
#include <assimp/Importer.hpp>
// local
#include "assets/model.h"
#include "assets/texture_manager.h"
#include "events.h"
#include "files.h"
#include "global.h"
#include "math/collision_types.h"
#include "opengl/buffer_storage.h"
#include "opengl/vertex_buffers.h"
// fwd
namespace ui {
class WinResources;
}

namespace gpu {

// cmd_index is packed [mesh_type, local_index] as two 16bit
struct StorageCmd {
  GLuint count;
  GLuint first_index;
  GLuint base_vertex;
  GLuint packed_cmd_index;
};

// unsorted commands, used to generate IndirectDrawCmd
struct IndirectCmdStorage {
  StorageCmd commands[global::kMaxDrawCommands];
};

// static mesh boxes are the same for all object's instances
struct StorageStaticBoxes {
  AABB boxes[global::kMaxDrawCommands];
};

struct StorageMaterials {
  Material materials[global::kMaxDrawCommands];
};

}  // namespace gpu

using MeshCounts = std::array<GLuint, MeshType::kTotal>;
using MeshOffsets = std::array<GLuint, MeshType::kTotal>;

class ModelManager : public event::Base<ModelManager> {
 public:
  ModelManager(TextureManager &textures, ui::WinResources &ui);
  using Inject = ModelManager(TextureManager &, ui::WinResources &);

 public:
  TextureManager &textures_;

  // multi-threaded
  gl::VertexBuffers mesh_static_;
  gl::VertexBuffers mesh_skinned_;

  // to invoke compute shader by Renderer
  bool build_indirect_cmd_{false};

  // each Model is created in the independent thread
  void LoadModelMt(const fs::path &path, unsigned int thread_id) noexcept;
  Model *FindModelMt(const std::string &name);

  const MeshCounts &GetMeshCounts() const;
  const MeshOffsets &GetMeshOffsets() const;
  GLuint GetMeshTotal() const;

  // prefer batch processing under mutex
  void UploadCmdBoxMaterial() noexcept;

  void UpdateMeshMaterial(Mesh &mesh);

 private:
  ui::WinResources &ui_;

  gl::CountedBuffer<gpu::IndirectCmdStorage> indirect_cmd_storage_{
      "IndirectCmdStorage"};
  gl::CountedBuffer<gpu::StorageStaticBoxes> static_boxes_{"StaticBoxes"};
  gl::CountedBuffer<gpu::StorageMaterials> materials_{"Materials"};

  mutable std::mutex mutex_;
  gl::Sync sync_;

  MeshCounts mesh_counts_{};
  MeshOffsets mesh_offsets_{};
  GLuint mesh_total_{0};

  GLuint tracker_material_{0};
  GLuint tracker_static_box_{0};

  MiVector<std::unique_ptr<Assimp::Importer>> workers_;
  MiUnMap<std::string, Model> models_;

  MiVector<Mesh *> queue_meshes_;
  MiVector<gpu::StorageCmd> upload_cmd_;
  MiVector<gpu::AABB> upload_static_box_;
  MiVector<gpu::Material> upload_material_;

  void InitEvents();
};
