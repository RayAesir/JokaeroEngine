#include "model_manager.h"

// deps
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <spdlog/spdlog.h>
// local
#include "app/parameters.h"
#include "ui/win_resources.h"
#include "utils/profiling.h"

ModelManager::ModelManager(TextureManager &textures, ui::WinResources &ui)
    : event::Base<ModelManager>(&ModelManager::InitEvents, this),
      textures_(textures),
      ui_(ui),
      mesh_static_({
          &kPositionAttr,   //
          &kNormalAttr,     //
          &kTangentAttr,    //
          &kBitangentAttr,  //
          &kTexCoordsAttr   //
      }),
      mesh_skinned_({
          &kPositionAttr,   //
          &kNormalAttr,     //
          &kTangentAttr,    //
          &kBitangentAttr,  //
          &kTexCoordsAttr,  //
          &kBonesAttr,      //
          &kWeightsAttr,    //
      }) {
  indirect_cmd_storage_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                                   ShaderStorageBinding::kIndirectCmdStorage);
  indirect_cmd_storage_.SetStorage();
  static_boxes_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                           ShaderStorageBinding::kStaticBoxes);
  static_boxes_.SetStorage();
  materials_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                        ShaderStorageBinding::kMaterials);
  materials_.SetStorage();

  mesh_static_.SetStorage(global::kMaxVerticesPerMemBlock,
                          global::kMaxIndicesPerMemBlock);
  mesh_skinned_.SetStorage(global::kMaxVerticesPerMemBlock,
                           global::kMaxIndicesPerMemBlock);

  workers_.reserve(app::cpu.task_threads);
  for (unsigned int i = 0; i < app::cpu.task_threads; ++i) {
    workers_.push_back(std::make_unique<Assimp::Importer>());
  }

  // prealloction
  queue_meshes_.reserve(global::kMaxDrawCommands / 32);
  upload_cmd_.reserve(global::kMaxDrawCommands / 32);
  upload_static_box_.reserve(global::kMaxDrawCommands / 32);
  upload_material_.reserve(global::kMaxDrawCommands / 32);

  ui_.loading_info_.models.reserve(app::cpu.task_threads);
  for (unsigned int i = 0; i < app::cpu.task_threads; ++i) {
    ui_.loading_info_.models.push_back(global::kEmptyName);
  }
}

void ModelManager::InitEvents() {
  Connect(Event::kApplyTextureSettings, [this]() {
    // create new handler for all textures in TextureManager
    textures_.ApplyTextureSettingsMainThread();
    // update the Materials in Meshes and upload to GPU
    for (auto &[name, model] : models_) {
      for (auto &mesh : model.meshes_) {
        UpdateMeshMaterial(mesh);
      }
    }
  });
}

void ModelManager::LoadModelMt(const fs::path &path,
                               unsigned int thread_id) noexcept {
  auto &importer = workers_[thread_id];
  // don't use those flags:
  // broke meshes
  // aiProcess_FindInvalidData
  // aiProcess_OptimizeMeshes
  // remove Blender Armature
  // aiProcess_PreTransformVertices
  // aiProcess_OptimizeGraph
  unsigned int assimp_flags = static_cast<unsigned int>(
      // frustum culling, selection
      aiProcess_GenBoundingBoxes
      // indexed geometry as standard (performance heavy)
      | aiProcess_JoinIdenticalVertices
      // skinning, access via aiBone
      | aiProcess_PopulateArmatureData
      // limit bone weights to 4 per vertex
      | aiProcess_LimitBoneWeights
      // debug and error
      | aiProcess_ValidateDataStructure);

  std::string filepath = path.string();
  std::string name = path.stem().string();

  ui_.loading_info_.models[thread_id] = name.c_str();

  prof::Counter read;
  const aiScene *scene = importer->ReadFile(filepath, assimp_flags);
  read.End();

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    spdlog::error("{}: Assimp_Importer {}", __FUNCTION__,
                  importer->GetErrorString());
    return;
  }

  prof::Counter upload;

  Model model{name, scene, *this};

  {
    std::scoped_lock lock(mutex_);
    auto [it, res] = models_.try_emplace(model.GetName(), std::move(model));
    auto &created = it->second;
    for (auto &mesh : created.meshes_) {
      queue_meshes_.push_back(&mesh);
    }
    ui_.table_model_.AddRow(created.GetId(), created.GetName().c_str());
  }
  upload.End();

  ui_.loading_info_.models[thread_id] = global::kEmptyName;

  // profiling (i7-6700k, RTX 3070 Ti)
  // assimp part, most time vertices(join)
  // engine part, ~95% of time images/textures
  spdlog::info(
      "Thread {}: {}, Assimp: {:.3f}s, Textures/Upload: {:.3f}s, Total: "
      "{:.3f}s",
      thread_id, name, read.GetTime<prof::fsec>(), upload.GetTime<prof::fsec>(),
      read.GetElapsed<prof::fsec>());
}

Model *ModelManager::FindModelMt(const std::string &name) {
  std::scoped_lock lock(mutex_);
  auto it = models_.find(name);
  if (it == models_.end()) {
    spdlog::warn("{}: Model is not found '{}'", __FUNCTION__, name);
    return nullptr;
  }
  return &it->second;
}

const MeshCounts &ModelManager::GetMeshCounts() const { return mesh_counts_; }

const MeshOffsets &ModelManager::GetMeshOffsets() const {
  return mesh_offsets_;
}

GLuint ModelManager::GetMeshTotal() const { return mesh_total_; }

void ModelManager::UpdateMeshMaterial(Mesh &mesh) {
  mesh.UpdateMaterialTextureHandlers();
  materials_.UploadIndex(mesh.material_, mesh.material_buffer_index_);
}

void ModelManager::UploadCmdBoxMaterial() noexcept {
  if (queue_meshes_.size() == 0) return;

  std::scoped_lock lock(mutex_);

  upload_cmd_.clear();
  upload_static_box_.clear();
  upload_material_.clear();

  for (auto *mesh : queue_meshes_) {
    GLuint mesh_type = mesh->GetType();
    GLuint local_index = mesh_counts_[mesh_type]++;
    GLuint packed_cmd_buffer_index = (mesh_type << 16) | local_index;
    mesh->packed_cmd_buffer_index_ = packed_cmd_buffer_index;
    // unordered draw commands
    const auto &addr = mesh->GetVertexAddr();
    upload_cmd_.emplace_back(addr.indice_count,       // count
                             addr.indice_offset,      // first_index
                             addr.vertex_offset,      // base_vertex
                             packed_cmd_buffer_index  //
    );
    // boxes (even == static)
    if ((mesh_type & 1) == 0) {
      auto &bb = mesh->GetBB();
      mesh->box_index_ = tracker_static_box_++;
      upload_static_box_.emplace_back(bb.center_, bb.extent_);
    }

    mesh->material_buffer_index_ = tracker_material_++;
    upload_material_.push_back(mesh->material_);
  }

  // OpenGL fence to protect upload
  sync_.BeginMt();
  indirect_cmd_storage_.AppendVector(upload_cmd_);
  if (upload_static_box_.size()) {
    static_boxes_.AppendVector(upload_static_box_);
  }
  materials_.AppendVector(upload_material_);
  sync_.EndMt();

  std::fill(mesh_offsets_.begin(), mesh_offsets_.end(), 0U);
  GLuint offset = 0;
  for (GLuint i = 0; i < MeshType::kTotal; ++i) {
    mesh_offsets_[i] = offset;
    offset += mesh_counts_[i];
  }
  mesh_total_ += static_cast<GLuint>(queue_meshes_.size());
  build_indirect_cmd_ = true;

  queue_meshes_.clear();
}
