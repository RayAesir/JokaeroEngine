#pragma once

// local
#include "assets/texture.h"
#include "events.h"
#include "global.h"
#include "opengl/buffer_storage.h"
#include "opengl/query.h"
#include "opengl/sampler.h"
#include "render/crosshair.h"
#include "render/framebuffers.h"
#include "render/generated_mesh.h"
#include "render/generated_texture.h"
#include "render/programs.h"
#include "render/uniform_buffers.h"
#include "render/vertex_arrays.h"
#include "ui/win_layout.h"
// fwd
class Assets;
class Scene;
class CameraSystem;
class ParticleSystem;
class Object;

namespace gpu {

// default OpenGL CMD
struct DrawElementsIndirectCommand {
  GLuint count;
  GLuint instance_count;
  GLuint first_index;
  GLuint base_vertex;
  GLuint base_instance;
};

// sorted commands (by mesh type) to draw
struct IndirectCmdDraw {
  DrawElementsIndirectCommand commands[global::kMaxDrawCommands];
};

// the overview of top 3 (by performance) instancing method:
// 1. UBO, the fastest method but memory limit
// 2. VBO (VAO), no limits
// 3. SSBO, slightly slower (~2-8%) than VBO because:

/*
https://stackoverflow.com/questions/33589784/ssbo-as-bigger-ubo
UBOs and SSBOs fundamentally represent two different pieces of hardware
(usually).

Shaders are executed in groups, such that every shader executes in lock-step.
Each group of individual shader invocations has access to a block of memory.
This memory is what UBOs represent. It is relatively small (on the order of
kilobytes), but accessing it is quite fast. When performing a rendering
operation, data from UBOs are copied into this shader local memory.

SSBOs represent global memory. They're basically pointers. That's why they
generally have no storage limitations (the minimum GL requirement is 16
Megabytes, with most implementations returning a number on the order of the
GPU's memory size).

They are slower to access, but this performance is because of where they exist
and how they're accessed, not because they might not be constant. Global memory
is global GPU memory rather than local constant memory.

If a shader needs to access more data than can comfortably fit in its shader
local memory, then it needs to use global memory. There's no getting around
this, even if you had a way to declare an SSBO to be "constant".
*/

// the additional data per instance (VAO), created by compute shaders
// Direct Shadowpass:
// uvec4(world matrix, material, animation, cascade index)
// Point Shadowpass:
// uvec4(world matrix, material, animation, cubemap layer index)
// View Renderpass:
// uvec4(world matrix, material, animation, instance index)
struct AttributeInstanceData {
  glm::uvec4 instance_data[global::kMaxInstances];
};

// precomputed matrices (proj_view * model), optimization
struct AttributeInstanceMatrixMvp {
  glm::mat4 mvp[global::kMaxInstances];
};

// resetted each frame, readback GPU -> CPU
struct StorageAtomicCounters {
  GLuint frustum_point_lights;
  GLuint frustum_point_shadows;
  GLuint cluster_point_lights;
  GLuint frustum_instances_direct_shadows;
  GLuint frustum_instances_point_shadows;
  GLuint frustum_instances_view;
  GLuint occlusion_instances_view;
  GLuint cmd_instance[global::kMaxDrawCommands];
};
inline constexpr GLsizeiptr kCmdCountersSize =
    offsetof(StorageAtomicCounters, cmd_instance);

// culled light's indices to access PointLight
struct StorageFrustumPointLights {
  GLuint frustum_point_lights[global::kMaxPointLights];
  GLuint frustum_point_shadows[global::kMaxPointShadows];
  GLfloat point_lights_shadow_index[global::kMaxPointLights];
};

struct StorageFrustumPointShadows {
  glm::mat4
      point_shadows_cubemap[global::kMaxPointShadows * global::kCubemapFaces];
  glm::vec4 point_shadows_pos_inv_rad2[global::kMaxPointShadows];
  glm::vec4 point_shadows_layer_frustum[global::kMaxPointShadows *
                                        global::kCubemapFaces]
                                       [global::kFrustumPlanes];
};

// clustered shading
struct ClusterAABB {
  glm::vec4 min_point;
  glm::vec4 max_point;
};

struct StorageFrustumClusters {
  ClusterAABB cluster_box[global::kFrustumClustersCount];
};

// all light's indices inside 1D array
struct StorageClusterLights {
  glm::uvec2 point_lights_count_offset[global::kFrustumClustersCount];
  GLuint point_lights_indices[global::kCulledPointLights];
};

struct StorageDebugReadback {
  GLint int_var[global::kDebugReadbackSize]{};
  GLuint uint_var[global::kDebugReadbackSize]{};
  GLfloat float_var[global::kDebugReadbackSize]{};
  glm::vec4 vec4_var[global::kDebugReadbackSize]{};
};

}  // namespace gpu

struct Metrics {
  enum Enum : unsigned int {
    kFrustumCullingPointLights,
    kClusterLightCulling,
    kFrustumCullingDirectShadows,
    kDirectShadowmap,
    kFrustumCullingPointShadows,
    kPointShadowmap,
    kFrustumCullingView,
    kPrepass,
    kOcclusionTexture,
    kFrustumCullingViewOcclusion,
    kPrepassAlphaBlend,
    kAlphaBlendTexture,
    kOutlineSelectedTexture,
    kGBuffer,
    kAlphaBlend,
    kParticles,
    kHbao,
    kGtao,
    kLighting,
    kSkybox,
    kCompositePassOIT,
    kOutlineSelectedDraw,
    kDebug,
    kBloom,
    kPostprocess,
    kFxaa,
    kTotal
  };
};

class Renderer : public event::Base<Renderer> {
 public:
  friend class ui::WinScene;
  Renderer(Assets& assets,            //
           Scene& scene,              //
           ui::Layout& ui,            //
           CameraSystem& cameras,     //
           ParticleSystem& particles  //
  );
  using Inject = Renderer(Assets&, Scene&, ui::Layout&, CameraSystem&,
                          ParticleSystem&);

 public:
  struct ProfilingQueries {
    int frame_accum{0};
    double accum[Metrics::kTotal]{};
    double time[Metrics::kTotal]{};
    double total_time{};
  };
  ProfilingQueries profiling_;

  void RenderLoading();
  void RenderScene();

  void RenderEnvironmentTexture(const gl::Texture2D& source,
                                std::shared_ptr<EnvTexture> env_tex);
  void RenderIBLArchive(const gl::Texture2D& skytex,       //
                        const gl::Texture2D& irradiance,   //
                        const gl::Texture2D& prefiltered,  //
                        std::shared_ptr<EnvTexture> env_tex);
  void GenerateBrdfLut();

 private:
  Assets& assets_;
  Scene& scene_;
  ui::Layout& ui_;

  CameraSystem& cameras_;
  ParticleSystem& particles_;

  gl::Buffer<gpu::IndirectCmdDraw> indirect_cmd_draw_{"IndirectCmdDraw"};

  // SSBO as VAO Attributes
  gl::Buffer<gpu::AttributeInstanceData> instance_attributes_{
      "AttributeInstanceData"};
  gl::Buffer<gpu::AttributeInstanceMatrixMvp> instance_matrix_mvp_{
      "AttributeInstanceMatrixMvp"};

  gl::MappedBuffer<gpu::StorageAtomicCounters> atomic_counters_{
      "AtomicCounters"};

  gl::Buffer<gpu::StorageFrustumPointLights> frustum_point_lights_{
      "FrustumPointLights"};
  gl::Buffer<gpu::StorageFrustumPointShadows> frustum_point_shadows_{
      "FrustumPointShadows"};
  gl::Buffer<gpu::StorageFrustumClusters> frustum_clusters_{"FrustumClusters"};
  gl::Buffer<gpu::StorageClusterLights> cluster_lights_{"ClusterLights"};

  gl::MappedBuffer<gpu::StorageDebugReadback> debug_readback_{"DebugReadback"};

  GeneratedMesh gen_mesh_;
  GeneratedTexture gen_tex_;
  Crosshair crosshair_;

  UniformBuffers ubo_;
  VertexArrays vao_;
  Framebuffers fb_;
  Programs prog_;

  gl::Sampler2D sampler_direct_shadow_;
  gl::Sampler3D sampler_point_shadow_;

  gl::Query queries_[Metrics::kTotal]{};
  gl::Sync sync_;

  // MultiDrawElementsIndirect (offset GLuint64 == uintptr_t -> GLvoid*)
  std::array<GLsizei, MeshType::kTotal> indirect_drawcount_;
  std::array<GLuint64, MeshType::kTotal> indirect_offsets_;

  void BindDefaultFramebuffer();
  void ProcessMetrics();

  void BuildIndirectCmd();
  void RenderMeshes(MeshType::Enum type, const gl::Program* prog,
                    const gl::VertexArray* vao);
  void RenderOpaqueAndAlphaClip(GLenum cull_face, const gl::Program* prog[],
                                const gl::VertexArray* vao[]);

  void FrustumCullingPointLights();
  void ClusterLightCulling();
  void DisablePointLights();

  void FrustumCulling(RenderPass::Enum pass);

  void DirectShadows();
  void DisableDirectShadows();

  void PointShadows();
  void DisablePointShadows();

  void PrepassAlphaBlend();
  void Prepass();
  void OcclusionAlphaBlendTexture();
  void OcclusionTexture();

  void OutlineSelectedTexture();
  void GBuffer();

  void BindBlendFramebuffer();
  void ForwardAlphaBlend();
  void ParticlesUpdate();
  void ParticlesDraw();

  void RenderHbao();
  void RenderGtao();

  void BindHdrFramebuffer();
  void DeferredLighting();
  void ShowShadowsDeferred();
  void ForwardLighting();
  void ShowShadowsForward();
  void Skybox();
  void CompositePassOIT();
  void OutlineSelectedDraw();

  void BindLdrFramebuffer();
  void Bloom();
  void Postprocess();
  void CrosshairOverlay();
  void DefaultFramebuffer();
  void ViewLayers();

  void DebugNormals();
  void DebugAxis();
  void DebugPointLights();
  void ShowPointLight();
  void DebugAABB();
  void DebugOBB();
  void DebugCameras();
  void DebugParticles();
  // place at specific line of code
  // to sync mapped debug buffer, has ui
  void GetDebugReadback();

  void InitEvents();
};