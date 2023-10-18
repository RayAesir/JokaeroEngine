#include "renderer.h"

// local
#include "app/input.h"
#include "app/parameters.h"
#include "assets/assets.h"
#include "assets/texture.h"
#include "glsl_header.h"
#include "math/random.h"
#include "options.h"
#include "scene/scene.h"

Renderer::Renderer(Assets& assets,            //
                   Scene& scene,              //
                   ui::Layout& ui,            //
                   CameraSystem& cameras,     //
                   ParticleSystem& particles  //
                   )
    : event::Base<Renderer>(&Renderer::InitEvents, this),
      assets_(assets),
      scene_(scene),
      ui_(ui),

      cameras_(cameras),
      particles_(particles),

      ubo_(assets, scene, cameras.camera_, ui.win_settings_),
      vao_(gen_mesh_.mesh_internal_,      //
           assets.models_.mesh_static_,   //
           assets.models_.mesh_skinned_,  //
           instance_attributes_,          //
           instance_matrix_mvp_) {
  //
  indirect_cmd_draw_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                                ShaderStorageBinding::kIndirectCmdDraw);
  indirect_cmd_draw_.SetStorage();

  // SSBO as VAO Attributes
  instance_attributes_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                                  ShaderStorageBinding::kAttributeInstanceData);
  instance_attributes_.SetStorage();

  instance_matrix_mvp_.BindBuffer(
      GL_SHADER_STORAGE_BUFFER,
      ShaderStorageBinding::kAttributeInstanceMatrixMvp);
  instance_matrix_mvp_.SetStorage();

  atomic_counters_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                              ShaderStorageBinding::kAtomicCounters);
  atomic_counters_.SetStorage(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                              GL_MAP_COHERENT_BIT);
  atomic_counters_.MapRange(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                            GL_MAP_COHERENT_BIT);

  frustum_point_lights_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                                   ShaderStorageBinding::kFrustumPointLights);
  frustum_point_lights_.SetStorage();

  frustum_point_shadows_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                                    ShaderStorageBinding::kFrustumPointShadows);
  frustum_point_shadows_.SetStorage();

  frustum_clusters_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                               ShaderStorageBinding::kFrustumClusters);
  frustum_clusters_.SetStorage();

  cluster_lights_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                             ShaderStorageBinding::kClusterLights);
  cluster_lights_.SetStorage();

  debug_readback_.BindBuffer(GL_SHADER_STORAGE_BUFFER,
                             ShaderStorageBinding::kDebugReadback);
  debug_readback_.SetStorage(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                             GL_MAP_COHERENT_BIT);
  debug_readback_.MapRange(GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT |
                           GL_MAP_COHERENT_BIT);

  sampler_direct_shadow_.SetFilter(GL_NEAREST, GL_NEAREST);
  sampler_direct_shadow_.SetWrap2D(GL_CLAMP_TO_BORDER);
  static const glm::vec4 border{1.0f, 0.0f, 0.0f, 0.0f};
  sampler_direct_shadow_.SetBorderColor(border);
  sampler_direct_shadow_.DisableCompareMode();

  sampler_point_shadow_.SetFilter(GL_NEAREST, GL_NEAREST);
  sampler_point_shadow_.SetWrap3D(GL_CLAMP_TO_EDGE);
  sampler_point_shadow_.DisableCompareMode();

  glsl::Header("header.glsl");
  gl::Program::InitAll();

  // some parameters are calculated only once
  ubo_.InitEngine();
  ubo_.InitHbao();
  ubo_.UpdateShadowsPoisson();
  ubo_.UploadBuffers();

  // set pointers to Debug buffer
  auto& debug = ui_.win_settings_.debug_readback_;
  debug.int_ptr = debug_readback_->int_var;
  debug.uint_ptr = debug_readback_->uint_var;
  debug.float_ptr = debug_readback_->float_var;
  debug.vec4_ptr = debug_readback_->vec4_var;
}

void Renderer::InitEvents() {
  Connect(Event::kReloadShaders, [this]() { gl::Program::ReloadAll(); });
  Connect(Event::kApplyResolution, [this]() { fb_.InitDefault(); });
  Connect(Event::kApplyDirShadowmapResolution,
          [this]() { fb_.InitDirectShadows(); });
  Connect(Event::kApplyPointShadowmapResolution,
          [this]() { fb_.InitPointShadows(); });
  Connect(Event::kUpdateShadowsPoisson,
          [this]() { ubo_.UpdateShadowsPoisson(); });
  Connect(Event::kGenerateBRDFLut, [this]() { GenerateBrdfLut(); });
}

// some interesting info
// https://computergraphics.stackexchange.com/questions/37/what-is-the-cost-of-changing-state/46#46

void Renderer::RenderEnvironmentTexture(const gl::Texture2D& source,
                                        std::shared_ptr<EnvTexture> env_tex) {
  // the depth test breaks the prefiltered map's layered rendering
  glDisable(GL_DEPTH_TEST);

  // environment
  glViewport(0, 0, opt::lighting.environment_map_size,
             opt::lighting.environment_map_size);
  fb_.environment.Bind();
  fb_.environment.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.environment.Clear(GL_DEPTH, 0, &global::kClearDepth);

  source.Bind(0);
  prog_.equirectangular_to_cubemap.Use();
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);
  // let OpenGL to generate mipmaps from first mip face
  // combatting visible dots artifact
  fb_.environment_map.GenerateMipMap();
  for (GLint level = 0; level < opt::lighting.prefilter_max_level; ++level) {
    GLsizei mip_size = opt::lighting.environment_map_size >> level;
    glCopyImageSubData(fb_.environment_map, GL_TEXTURE_CUBE_MAP, level, 0, 0, 0,
                       env_tex->environment_tbo_, GL_TEXTURE_CUBE_MAP, level, 0,
                       0, 0, mip_size, mip_size, 6);
  }

  // irradiance
  glViewport(0, 0, opt::lighting.irradiance_map_size,
             opt::lighting.irradiance_map_size);
  fb_.irradiance.Bind();
  fb_.irradiance.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.irradiance.Clear(GL_DEPTH, 0, &global::kClearDepth);

  fb_.environment_map.Bind(0);
  prog_.irradiance_convolution.Use();
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);
  glCopyImageSubData(fb_.irradiance_map, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                     env_tex->irradiance_tbo_, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                     opt::lighting.irradiance_map_size,
                     opt::lighting.irradiance_map_size, 6);

  // prefiltered environment
  fb_.prefilter.Bind();
  fb_.prefilter.Clear(GL_COLOR, 0, global::kClearWhite);
  fb_.prefilter.Clear(GL_DEPTH, 0, &global::kClearDepth);

  fb_.environment_map.Bind(0);
  prog_.prefilter_environment.Use();
  for (GLint level = 0; level < opt::lighting.prefilter_max_level; ++level) {
    GLsizei mip_size = opt::lighting.prefilter_map_size >> level;
    GLfloat roughness =
        static_cast<GLfloat>(level) /
        static_cast<GLfloat>(opt::lighting.prefilter_max_level - 1);
    glViewport(0, 0, mip_size, mip_size);
    // render to selected level, depth attachment for framebuffer completness
    fb_.prefilter.AttachTexture(GL_COLOR_ATTACHMENT0, fb_.prefilter_map, level);
    fb_.prefilter.AttachTexture(GL_DEPTH_ATTACHMENT, fb_.prefilter_depth,
                                level);
    prog_.prefilter_environment.SetUniform("uRoughness", roughness);
    gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);
  }
  for (GLint level = 0; level < opt::lighting.prefilter_max_level; ++level) {
    GLsizei mip_size = opt::lighting.prefilter_map_size >> level;
    glCopyImageSubData(fb_.prefilter_map, GL_TEXTURE_CUBE_MAP, level, 0, 0, 0,
                       env_tex->prefiltered_tbo_, GL_TEXTURE_CUBE_MAP, level, 0,
                       0, 0, mip_size, mip_size, 6);
  }
  glEnable(GL_DEPTH_TEST);

  BindDefaultFramebuffer();
}

void Renderer::RenderIBLArchive(const gl::Texture2D& skytex,       //
                                const gl::Texture2D& irradiance,   //
                                const gl::Texture2D& prefiltered,  //
                                std::shared_ptr<EnvTexture> env_tex) {
  glDisable(GL_DEPTH_TEST);

  // environment
  glViewport(0, 0, opt::lighting.environment_map_size,
             opt::lighting.environment_map_size);
  fb_.environment.Bind();
  fb_.environment.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.environment.Clear(GL_DEPTH, 0, &global::kClearDepth);

  skytex.Bind(0);
  prog_.equirectangular_to_cubemap.Use();
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);

  fb_.environment_map.GenerateMipMap();
  for (GLint level = 0; level < opt::lighting.prefilter_max_level; ++level) {
    GLsizei mip_size = opt::lighting.environment_map_size >> level;
    glCopyImageSubData(fb_.environment_map, GL_TEXTURE_CUBE_MAP, level, 0, 0, 0,
                       env_tex->environment_tbo_, GL_TEXTURE_CUBE_MAP, level, 0,
                       0, 0, mip_size, mip_size, 6);
  }

  // irradiance
  glViewport(0, 0, opt::lighting.irradiance_map_size,
             opt::lighting.irradiance_map_size);
  fb_.environment.Bind();
  fb_.environment.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.environment.Clear(GL_DEPTH, 0, &global::kClearDepth);

  irradiance.Bind(0);
  prog_.equirectangular_to_cubemap.Use();
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);

  {
    GLsizei mip_size = opt::lighting.irradiance_map_size;
    glCopyImageSubData(fb_.environment_map, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                       env_tex->irradiance_tbo_, GL_TEXTURE_CUBE_MAP, 0, 0, 0,
                       0, mip_size, mip_size, 6);
  }

  // prefiltered
  glViewport(0, 0, opt::lighting.prefilter_map_size,
             opt::lighting.prefilter_map_size);
  fb_.environment.Bind();
  fb_.environment.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.environment.Clear(GL_DEPTH, 0, &global::kClearDepth);

  prefiltered.Bind(0);
  prog_.equirectangular_to_cubemap.Use();
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);

  fb_.environment_map.GenerateMipMap();
  for (GLint level = 0; level < opt::lighting.prefilter_max_level; ++level) {
    GLsizei mip_size = opt::lighting.prefilter_map_size >> level;
    glCopyImageSubData(fb_.environment_map, GL_TEXTURE_CUBE_MAP, level, 0, 0, 0,
                       env_tex->prefiltered_tbo_, GL_TEXTURE_CUBE_MAP, level, 0,
                       0, 0, mip_size, mip_size, 6);
  }
  glEnable(GL_DEPTH_TEST);

  BindDefaultFramebuffer();
}

void Renderer::GenerateBrdfLut() {
  glDisable(GL_DEPTH_TEST);

  glViewport(0, 0, opt::lighting.brdf_lut_map_size,
             opt::lighting.brdf_lut_map_size);
  fb_.brdf.Bind();
  fb_.brdf.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.brdf.Clear(GL_DEPTH, 0, &global::kClearDepth);

  prog_.brdf_lut.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glEnable(GL_DEPTH_TEST);

  BindDefaultFramebuffer();
}

void Renderer::BindDefaultFramebuffer() {
  // there we have two options:
  // 1. set glViewport() to the default FBO resolution and render
  // 2. use glBlitFramebuffer()
  // the first works well for windowed mode
  // the second works well for dynamic resolution
  glViewport(0, 0, app::glfw.window_size.x, app::glfw.window_size.y);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearNamedFramebufferfv(0, GL_COLOR, 0, global::kClearBlackAlphaOne);
  glClearNamedFramebufferfv(0, GL_DEPTH, 0, &global::kClearDepth);
}

void Renderer::ProcessMetrics() {
  auto& p = profiling_;
  // ProcessMetrics
  if (p.frame_accum == 20) {
    p.total_time = 0.0;
    for (int q = 0; q < Metrics::kTotal; ++q) {
      p.time[q] = p.accum[q] / 20.0;
      p.accum[q] = 0.0;
      p.total_time += p.time[q];
    }
    p.frame_accum = 0;
  } else {
    for (int q = 0; q < Metrics::kTotal; ++q) {
      p.accum[q] += queries_[q].GetResultAsTime();
    }
  }
  ++p.frame_accum;
}

void Renderer::BuildIndirectCmd() {
  std::fill(indirect_drawcount_.begin(), indirect_drawcount_.end(), 0);
  std::fill(indirect_offsets_.begin(), indirect_offsets_.end(), 0ULL);
  const auto& counts = assets_.models_.GetMeshCounts();
  const auto& offsets = assets_.models_.GetMeshOffsets();
  for (GLuint i = 0; i < MeshType::kTotal; ++i) {
    indirect_drawcount_[i] = static_cast<GLsizei>(counts[i]);
    // calculate offsets (in bytes)
    indirect_offsets_[i] = static_cast<GLuint64>(offsets[i]) *
                           sizeof(gpu::DrawElementsIndirectCommand);
  }

  GLuint workgroups =
      CeilInt(assets_.models_.GetMeshTotal(), app::gpu.subgroup_size);
  prog_.cmd_build.Use();
  glDispatchCompute(workgroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::RenderMeshes(MeshType::Enum type, const gl::Program* prog,
                            const gl::VertexArray* vao) {
  const auto& cmds = assets_.models_.GetMeshCounts();
  if (cmds[type]) {
    prog->Use();
    vao->Bind();
    glMultiDrawElementsIndirect(
        GL_TRIANGLES,                                        //
        GL_UNSIGNED_INT,                                     //
        reinterpret_cast<GLvoid*>(indirect_offsets_[type]),  //
        indirect_drawcount_[type],                           //
        0                                                    //
    );
  }
}

void Renderer::RenderOpaqueAndAlphaClip(GLenum cull_face,
                                        const gl::Program* prog[],
                                        const gl::VertexArray* vao[]) {
  using enum MeshType::Enum;

  glEnable(GL_CULL_FACE);
  glCullFace(cull_face);
  RenderMeshes(kStaticOpaque, prog[kStaticOpaque], vao[kStaticOpaque]);
  RenderMeshes(kSkinnedOpaque, prog[kSkinnedOpaque], vao[kSkinnedOpaque]);
  glDisable(GL_CULL_FACE);

  RenderMeshes(kStaticAlphaClip, prog[kStaticAlphaClip], vao[kStaticAlphaClip]);
  RenderMeshes(kSkinnedAlphaClip, prog[kSkinnedAlphaClip],
               vao[kSkinnedAlphaClip]);
}

void Renderer::FrustumCullingPointLights() {
  // the stream compaction for light culling + shadows bitonic sort
  // is faster than use bitonic sort for lights+shadows, ~2x times
  prog_.frustum_culling_point_lights.Use();
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

  // point lights/shadows count
  sync_.FenceSt();

  // copy point lights/shadows count from SSBO into uniform (optimization)
  ubo_.buffer_.SubData(UniformBinding::kLighting, sizeof(GLuint) * 2,
                       atomic_counters_);

  prog_.pick_point_light.Use();
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  if (atomic_counters_->frustum_point_shadows) {
    prog_.sort_point_shadowmaps.Use();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    prog_.create_point_shadowmaps.Use();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}

void Renderer::ClusterLightCulling() {
  // based on:
  // https://www.3dgep.com/wp-content/uploads/2017/07/3910539_Jeremiah_van_Oosten_Volume_Tiled_Forward_Shading.pdf
  // tested with lighting BVH, got performance overhead 0.250ms vs 0.100ms
  // BVH works well with huge amount of lights (>4k or more)
  // therefore some steps are skipped:
  // - mark active lights (depth pre-pass)
  // - build cluster list
  // - assign lights to clusters
  if (atomic_counters_->frustum_point_lights) {
    static constexpr glm::uvec3 cluster_workgroups =
        global::kFrustumClusters / global::kFrustumClustersThreads;

    // prepare AABB (recalculate when fov or near/far changed)
    if (scene_.camera_.update_clusters_) {
      prog_.create_frustum_clusters.Use();
      glDispatchCompute(cluster_workgroups.x, cluster_workgroups.y,
                        cluster_workgroups.z);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      scene_.camera_.update_clusters_ = false;
    }

    // test all frustum lights with all clusters
    prog_.cluster_light_culling.Use();
    glDispatchCompute(cluster_workgroups.x, cluster_workgroups.y,
                      cluster_workgroups.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}

void Renderer::DisablePointLights() {
  // all zeros, disabled
  frustum_point_lights_.ClearData();
  frustum_point_shadows_.ClearData();
  cluster_lights_.ClearData();
  sync_.FenceSt();
  ubo_.buffer_.SubData(UniformBinding::kLighting, sizeof(GLuint) * 2,
                       atomic_counters_);
}

void Renderer::FrustumCulling(RenderPass::Enum pass) {
  GLuint instance_count = scene_.instance_count_;
  GLuint mesh_workgroups =
      CeilInt(assets_.models_.GetMeshTotal(), app::gpu.subgroup_size);
  GLuint instance_workgroups = CeilInt(instance_count, app::gpu.subgroup_size);
  // reset instance counters
  atomic_counters_.ClearSubData(gpu::kCmdCountersSize,
                                sizeof(GLuint) * instance_count);

  prog_.cmd_reset_instance_count.Use();
  glDispatchCompute(mesh_workgroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  prog_.frustum_culling[pass]->Use();
  glDispatchCompute(instance_workgroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  prog_.cmd_set_base_instance.Use();
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  prog_.instance_compact[pass]->Use();
  glDispatchCompute(instance_workgroups, 1, 1);
  glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void Renderer::DirectShadows() {
  int shadowmap_size = opt::shadows.direct_shadowmap.Selected().size;
  // some methods to remove shadow acne:
  // 1. apply the slope-scaled bias in the fragment shader that is
  // appropriately scaled for cascade (remove the flickering between cascades)
  // 2. use glPolygonOffset() that takes into account a polygon slope
  // and equal around all shadow maps (smooth transiting)
  // 3. calculate a per-texel depth bias with DDX and DDY (not used here)
  glViewport(0, 0, shadowmap_size, shadowmap_size);
  fb_.shadows_direct.Bind();
  fb_.shadows_direct.Clear(GL_DEPTH, 0, &global::kClearDepth);

  glEnable(GL_DEPTH_CLAMP);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(opt::shadows.polygon_offset_factor,
                  opt::shadows.polygon_offset_units);

  RenderOpaqueAndAlphaClip(GL_FRONT, prog_.shadows_dir, vao_.direct_shadowpass);

  glPolygonOffset(0.0f, 0.0f);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_DEPTH_CLAMP);
}

void Renderer::DisableDirectShadows() {
  fb_.shadows_direct.Bind();
  fb_.shadows_direct.Clear(GL_DEPTH, 0, &global::kClearDepth);
}

void Renderer::PointShadows() {
  int shadowmap_size = opt::shadows.point_shadowmap.Selected().size;
  glViewport(0, 0, shadowmap_size, shadowmap_size);
  fb_.shadows_point.Bind();

  GLint shadows_count =
      static_cast<GLint>(atomic_counters_->frustum_point_shadows);
  for (GLint index = 0; index < shadows_count; ++index) {
    GLint start_face = index * 6;
    constexpr GLint level = 0;
    constexpr GLint xoffset = 0;
    constexpr GLint yoffset = 0;
    constexpr GLsizei depth = 6;
    glClearTexSubImage(fb_.shadowmap_point,  //
                       level,                //
                       xoffset,              //
                       yoffset,              //
                       start_face,           //
                       shadowmap_size,       //
                       shadowmap_size,       //
                       depth,                //
                       GL_DEPTH_COMPONENT, GL_FLOAT, &global::kClearDepth);
  }

  RenderOpaqueAndAlphaClip(GL_FRONT, prog_.shadows_point,
                           vao_.point_shadowpass);
}

void Renderer::DisablePointShadows() {
  int shadowmap_size = opt::shadows.point_shadowmap.Selected().size;
  fb_.shadows_point.Bind();
  GLint shadows_count =
      static_cast<GLint>(atomic_counters_->frustum_point_shadows);
  for (GLint index = 0; index < shadows_count; ++index) {
    GLint start_face = index * 6;
    constexpr GLint level = 0;
    constexpr GLint xoffset = 0;
    constexpr GLint yoffset = 0;
    constexpr GLsizei depth = 6;
    glClearTexSubImage(fb_.shadowmap_point,  //
                       level,                //
                       xoffset,              //
                       yoffset,              //
                       start_face,           //
                       shadowmap_size,       //
                       shadowmap_size,       //
                       depth,                //
                       GL_DEPTH_COMPONENT, GL_FLOAT, &global::kClearDepth);
  }
}

void Renderer::PrepassAlphaBlend() {
  const auto& opengl = app::opengl;
  glViewport(0, 0, opengl.framebuffer_size.x, opengl.framebuffer_size.y);
  fb_.prepass_alpha_blend.Bind();
  // reverse depth
  fb_.prepass_alpha_blend.Clear(GL_COLOR, 0, global::kClearWhite);
  // instance indices (position in buffer)
  fb_.prepass_alpha_blend.Clear(GL_COLOR, 1, &global::kZero4Bytes);
  fb_.prepass_alpha_blend.Clear(GL_DEPTH, 0, &global::kClearReverseDepth);

  using enum MeshType::Enum;
  RenderMeshes(kStaticAlphaBlend, prog_.prepass[kStaticAlphaBlend],
               vao_.prepass[kStaticAlphaBlend]);
  RenderMeshes(kSkinnedAlphaBlend, prog_.prepass[kSkinnedAlphaBlend],
               vao_.prepass[kSkinnedAlphaBlend]);
}

void Renderer::Prepass() {
  const auto& opengl = app::opengl;
  glViewport(0, 0, opengl.framebuffer_size.x, opengl.framebuffer_size.y);
  fb_.prepass.Bind();
  // reverse depth
  fb_.prepass.Clear(GL_COLOR, 0, global::kClearWhite);
  // instance indices (position in buffer)
  fb_.prepass.Clear(GL_COLOR, 1, &global::kZero4Bytes);
  fb_.prepass.Clear(GL_DEPTH, 0, &global::kClearReverseDepth);

  RenderOpaqueAndAlphaClip(GL_BACK, prog_.prepass, vao_.prepass);
}

void Renderer::OcclusionAlphaBlendTexture() {
  // used only to pick object, because objects are fully transparent
  glm::uvec2 tex_workgroups =
      CeilInt(app::opengl.framebuffer_size, glm::ivec2(app::gpu.subgroup_size));

  fb_.prepass_ab_linear_depth.Bind(0);
  fb_.prepass_ab_occlusion.Bind(1);
  prog_.occlusion_alpha_blend_texture.Use();
  glDispatchCompute(tex_workgroups.x, tex_workgroups.y, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::OcclusionTexture() {
  glm::uvec2 tex_workgroups =
      CeilInt(app::opengl.framebuffer_size, glm::ivec2(app::gpu.subgroup_size));
  GLuint instance_workgroups =
      CeilInt(scene_.instance_count_, app::gpu.subgroup_size);

  // reset visibility for opaque and alpha clip types
  // instances are uploaded in random order, use shader
  prog_.instance_reset_visibility.Use();
  glDispatchCompute(instance_workgroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  fb_.prepass_linear_depth.Bind(0);
  fb_.prepass_occlusion.Bind(1);

  // read instance indices from texture (Prepass)
  // pick opaque objects
  prog_.occlusion_culling_texture.Use();
  glDispatchCompute(tex_workgroups.x, tex_workgroups.y, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::OutlineSelectedTexture() {
  auto* object = scene_.selected_object_;
  if ((object != nullptr) && (object->IsVisible())) {
    fb_.outline.Bind();
    fb_.outline.Clear(GL_COLOR, 0, global::kClearBlackAlphaOne);
    fb_.outline.Clear(GL_DEPTH, 0, &global::kClearReverseDepth);

    const auto& meshes = object->GetModel().meshes_;
    if (object->IsAnimated()) {
      vao_.pos_norm_tex_skin_inst_mvp.Bind();
      for (const auto& mesh : meshes) {
        const auto* outline = prog_.outline_selected_texture[mesh.GetType()];
        outline->Use();
        outline->SetUniform("uModel", object->GetTransformation());
        if (mesh.GetType() == MeshType::kSkinnedAlphaClip) {
          outline->SetUniform("uMaterialIndex", mesh.material_buffer_index_);
        }
        outline->SetUniform("uAnimationIndex",
                            object->GetAddr().animation_index);
        mesh.GetVertexAddr().Draw(GL_TRIANGLES);
      }
    } else {
      vao_.pos_norm_tex_inst_mvp.Bind();
      for (const auto& mesh : meshes) {
        const auto* outline = prog_.outline_selected_texture[mesh.GetType()];
        outline->Use();
        outline->SetUniform("uModel", object->GetTransformation());
        if (mesh.GetType() == MeshType::kStaticAlphaClip) {
          outline->SetUniform("uMaterialIndex", mesh.material_buffer_index_);
        }
        mesh.GetVertexAddr().Draw(GL_TRIANGLES);
      }
    }
  }
}

void Renderer::GBuffer() {
  glViewport(0, 0, app::opengl.framebuffer_size.x,
             app::opengl.framebuffer_size.y);
  fb_.gbuffer.Bind();
  fb_.gbuffer.Clear(GL_COLOR, 0, global::kClearWhite);
  fb_.gbuffer.Clear(GL_COLOR, 1, global::kClearBlack);
  fb_.gbuffer.Clear(GL_COLOR, 2, global::kClearBlack);

  RenderOpaqueAndAlphaClip(GL_BACK, prog_.gbuffer, vao_.viewpass);
}

void Renderer::BindBlendFramebuffer() {
  glViewport(0, 0, app::opengl.framebuffer_size.x,
             app::opengl.framebuffer_size.y);
  fb_.transparency.Bind();
  fb_.transparency.Clear(GL_COLOR, 0, global::kClearBlack);
  fb_.transparency.Clear(GL_COLOR, 1, global::kClearWhite);
}

void Renderer::ForwardAlphaBlend() {
  scene_.env_tex_->irradiance_tbo_.Bind(0);
  scene_.env_tex_->prefiltered_tbo_.Bind(1);
  fb_.brdf_lut.Bind(2);

  using enum MeshType::Enum;
  RenderMeshes(kStaticAlphaBlend, &prog_.forward_blend,
               vao_.viewpass[kStaticAlphaBlend]);
  RenderMeshes(kSkinnedAlphaBlend, &prog_.forward_blend_skinned,
               vao_.viewpass[kSkinnedAlphaBlend]);
}

void Renderer::ParticlesUpdate() {
  prog_.particles_update.Use();
  prog_.particles_update.SetUniform("uDeltaT", app::glfw.delta_time);
  for (const FxPreset* fx : particles_.GetFxToDraw()) {
    prog_.particles_update.SetUniform("uRandom", rnd::GetDefaultVec3());
    prog_.particles_update.SetUniform("uFxId", fx->buffer_index);

    GLuint workgroups = CeilInt(fx->num_of_particles, app::gpu.subgroup_size);
    glDispatchCompute(workgroups, 1, 1);
  }
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::ParticlesDraw() {
  prog_.particles_render.Use();
  particles_.DrawParticles();
}

void Renderer::RenderHbao() {
  // deinterleave depth (quater res)
  glViewport(0, 0, opt::hbao.quater_size.x, opt::hbao.quater_size.y);
  fb_.hbao_deinterleave.Bind();

  fb_.prepass_linear_depth.Bind(0);
  prog_.hbao_deinterleave.Use();
  // two passes, 16 layers and max 8 drawbuffers
  for (int i = 0; i < global::kRandomElements; i += app::gpu.max_drawbuffers) {
    glm::vec2 offset_uv{static_cast<GLfloat>(i % 4) + 0.5f,
                        static_cast<GLfloat>(i / 4) + 0.5f};
    prog_.hbao_deinterleave.SetUniform("uOffsetUv", offset_uv);
    for (int layer = 0; layer < app::gpu.max_drawbuffers; ++layer) {
      glNamedFramebufferTextureLayer(fb_.hbao_deinterleave,
                                     GL_COLOR_ATTACHMENT0 + layer,
                                     fb_.hbao_depth_array, 0, i + layer);
    }
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // calculate occlusion, for 16 layers (quater res)
  fb_.hbao_compute.Bind();
  fb_.hbao_compute.Clear(GL_COLOR, 0, global::kClearWhite);

  fb_.hbao_depth_array.Bind(0);
  prog_.hbao.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3 * global::kRandomElements);

  // interleave (fullscreen res)
  glViewport(0, 0, app::opengl.framebuffer_size.x,
             app::opengl.framebuffer_size.y);
  fb_.hbao_interleave.Bind();
  fb_.hbao_interleave.Clear(GL_COLOR, 0, global::kClearBlack);

  fb_.hbao_result_array.Bind(0);
  prog_.hbao_interleave.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);

  fb_.hbao_blur.Bind();
  // horizontal
  fb_.hbao_blur.SetDrawBuffer(GL_COLOR_ATTACHMENT0);
  fb_.hbao_interleaved.Bind(0);
  prog_.hbao_blur_horizontal.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);
  // vertical
  fb_.hbao_blur.SetDrawBuffer(GL_COLOR_ATTACHMENT1);
  fb_.hbao_blur_horizontal.Bind(0);
  prog_.hbao_blur_vertical.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::RenderGtao() {
  fb_.gtao.Bind();
  fb_.gtao.Clear(GL_COLOR, 0, global::kClearWhite);
  fb_.gtao.Clear(GL_COLOR, 1, global::kClearBlack);

  fb_.prepass_linear_depth.Bind(0);
  gen_tex_.hilbert_indices_.Bind(1);
  prog_.gtao.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);

  // denoise with compute shader
  // we're computing 2 horizontal pixels at a time (performance optimization)
  const glm::uvec2 workgroups =
      CeilInt(app::opengl.framebuffer_size, glm::ivec2(8 * 2, 8));
  fb_.empty.Bind();

  fb_.gtao_edges.Bind(1);
  prog_.gtao_denoise.Use();

  fb_.gtao_normals_visibility.Bind(0);
  glBindImageTexture(0, fb_.gtao_denoise, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_R32UI);
  prog_.gtao_denoise.SetUniform("uFinalApply", false);
  glDispatchCompute(workgroups.x, workgroups.y, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                  GL_TEXTURE_FETCH_BARRIER_BIT);

  if (opt::gtao.two_pass_blur) {
    fb_.gtao_denoise.Bind(0);
    glBindImageTexture(0, fb_.gtao_denoise_final, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_R32UI);
    prog_.gtao_denoise.SetUniform("uFinalApply", false);
    glDispatchCompute(workgroups.x, workgroups.y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT);

    fb_.gtao_denoise_final.Bind(0);
    glBindImageTexture(0, fb_.gtao_denoise, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_R32UI);
    prog_.gtao_denoise.SetUniform("uFinalApply", false);
    glDispatchCompute(workgroups.x, workgroups.y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT);
  }

  fb_.gtao_denoise.Bind(0);
  glBindImageTexture(0, fb_.gtao_denoise_final, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_R32UI);
  prog_.gtao_denoise.SetUniform("uFinalApply", true);
  glDispatchCompute(workgroups.x, workgroups.y, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                  GL_TEXTURE_FETCH_BARRIER_BIT);
}

void Renderer::BindHdrFramebuffer() {
  glViewport(0, 0, app::opengl.framebuffer_size.x,
             app::opengl.framebuffer_size.y);
  fb_.hdr.Bind();
  fb_.hdr.Clear(GL_COLOR, 0, global::kClearBlackAlphaOne);
}

void Renderer::DeferredLighting() {
  fb_.prepass_linear_depth.Bind(0);
  fb_.gbuffer_normals.Bind(1);
  fb_.gbuffer_color.Bind(2);
  fb_.gbuffer_pbr.Bind(3);
  scene_.env_tex_->irradiance_tbo_.Bind(4);
  scene_.env_tex_->prefiltered_tbo_.Bind(5);
  fb_.brdf_lut.Bind(6);
  fb_.shadowmap_direct.Bind(7);
  fb_.shadowmap_point.Bind(8);
  gen_tex_.shadow_random_angles_.Bind(9);
  fb_.hbao_blur_vertical_final.Bind(10);
  fb_.gtao_denoise_final.Bind(11);

  prog_.deferred_lighting.Use();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::ShowShadowsDeferred() {
  fb_.prepass_linear_depth.Bind(0);
  fb_.gbuffer_normals.Bind(1);
  fb_.shadowmap_direct.Bind(2);
  fb_.shadowmap_point.Bind(3);
  gen_tex_.shadow_random_angles_.Bind(4);

  prog_.show_shadows_deferred.Use();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::ForwardLighting() {
  scene_.env_tex_->irradiance_tbo_.Bind(0);
  scene_.env_tex_->prefiltered_tbo_.Bind(1);
  fb_.brdf_lut.Bind(2);
  fb_.shadowmap_direct.Bind(3);
  fb_.shadowmap_point.Bind(4);
  gen_tex_.shadow_random_angles_.Bind(5);
  fb_.hbao_blur_vertical_final.Bind(6);
  fb_.gtao_denoise_final.Bind(7);

  RenderOpaqueAndAlphaClip(GL_BACK, prog_.forward, vao_.viewpass);
}

void Renderer::ShowShadowsForward() {
  fb_.shadowmap_direct.Bind(0);
  fb_.shadowmap_point.Bind(1);
  gen_tex_.shadow_random_angles_.Bind(2);

  RenderOpaqueAndAlphaClip(GL_BACK, prog_.show_shadows_forward, vao_.viewpass);
}

void Renderer::Skybox() {
  scene_.env_tex_->environment_tbo_.Bind(0);
  if (opt::lighting.show_irradiance) {
    scene_.env_tex_->irradiance_tbo_.Bind(0);
  }
  if (opt::lighting.show_prefiltered) {
    scene_.env_tex_->prefiltered_tbo_.Bind(0);
  }
  prog_.skybox.Use();
  prog_.skybox.SetUniform("uLod", opt::lighting.cubemap_lod);
  gen_mesh_.DrawInternalMesh(GenMeshInternal::kSkybox, GL_TRIANGLES);
}

void Renderer::CompositePassOIT() {
  fb_.color_accum.Bind(0);
  fb_.revealage.Bind(1);

  prog_.composite_pass.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::OutlineSelectedDraw() {
  auto* object = scene_.selected_object_;
  if ((object != nullptr) && (object->IsVisible())) {
    fb_.outline_normals_depth.Bind(0);
    prog_.outline_selected.Use();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}

void Renderer::BindLdrFramebuffer() {
  fb_.ldr.Bind();
  fb_.ldr.Clear(GL_COLOR, 0, global::kClearBlackAlphaOne);
}

void Renderer::Bloom() {
  // based on
  // https://github.com/Shot511/RapidGL/tree/master/src/demos/26_bloom
  const auto& pp = opt::postprocess;
  const auto& bloom_downscale = prog_.bloom_downscale;
  const auto& bloom_upscale = prog_.bloom_upscale;
  const auto& mipmaps = fb_.bloom_mipmaps;
  GLint mip_levels = fb_.bloom_mip_levels;

  fb_.hdr_map.Bind(0);

  bloom_downscale.Use();
  glm::vec4 threshold{pp.bloom_threshold, pp.bloom_threshold - pp.bloom_knee,
                      2.0f * pp.bloom_knee, 0.25f * pp.bloom_knee};
  bloom_downscale.SetUniform("uThreshold", threshold);
  for (GLint level = 0, sub_level = 1; level < mip_levels;
       ++level, ++sub_level) {
    bloom_downscale.SetUniform("uTexelSize", mipmaps[sub_level].texel_size);
    bloom_downscale.SetUniform("uMipLevel", level);
    // use quadratic threshold for base level
    bloom_downscale.SetUniform("uUseThreshold", level == 0);
    glBindImageTexture(0, fb_.hdr_map, sub_level, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_RGBA16F);
    glDispatchCompute(mipmaps[sub_level].workgroups.x,
                      mipmaps[sub_level].workgroups.y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT);
  }

  bloom_upscale.Use();
  bloom_upscale.SetUniform("uBloomIntensity", pp.bloom_intensity);
  for (GLint level = mip_levels, top_level = mip_levels - 1; level >= 1;
       --level, --top_level) {
    bloom_upscale.SetUniform("uTexelSize", mipmaps[top_level].texel_size);
    bloom_upscale.SetUniform("uMipLevel", level);
    glBindImageTexture(0, fb_.hdr_map, top_level, GL_FALSE, 0, GL_READ_WRITE,
                       GL_RGBA16F);
    glDispatchCompute(mipmaps[top_level].workgroups.x,
                      mipmaps[top_level].workgroups.y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT);
  }

  glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
}

void Renderer::Postprocess() {
  fb_.hdr_map.Bind(0);
  prog_.postprocess.Use();
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::CrosshairOverlay() {
  prog_.crosshair.Use();
  crosshair_.UpdateCrosshair();
  crosshair_.DrawCrosshair();
}

void Renderer::DefaultFramebuffer() {
  // FXAA is engineered to be applied towards the end of engine post processing
  // after conversion to low dynamic range and conversion to the sRGB color
  // space for display
  fb_.ldr_luma.Bind(0);
  if (opt::postprocess.fxaa) {
    if (opt::postprocess.fxaa_quality) {
      prog_.fxaa_quality.Use();
    } else {
      prog_.fxaa_performance.Use();
    }
  } else {
    prog_.default_framebuffer.Use();
  }
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::ViewLayers() {
  struct Format {
    enum Enum : int {
      kRgb,
      kRed,
      kGreen,
      kBlue,
      kAlpha,
      kDepthPrepass,
      kDepthOutline,
      kRgbHdr
    };
  };

  auto show_layer = [this](int format, GLuint tbo) {
    glBindTextureUnit(0, tbo);
    prog_.draw_texture.Use();
    prog_.draw_texture.SetUniform("uFormat", format);
    prog_.draw_texture.SetUniform("uMipLevel", 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  };

  auto show_uint_layer = [this](int format, GLuint tbo) {
    glBindTextureUnit(0, tbo);
    prog_.draw_uint_texture.Use();
    prog_.draw_uint_texture.SetUniform("uFormat", format);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  };

  auto show_layer_mipmap = [this](int format, float mipmap, GLuint tbo) {
    glBindTextureUnit(0, tbo);
    prog_.draw_texture.Use();
    prog_.draw_texture.SetUniform("uFormat", format);
    prog_.draw_texture.SetUniform("uMipLevel", mipmap);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  };

  auto show_layer_csm = [this](int cascade, GLuint tbo) {
    sampler_direct_shadow_.Bind(0);
    glBindTextureUnit(0, tbo);
    prog_.draw_texture_csm.Use();
    prog_.draw_texture_csm.SetUniform("uCascade", cascade);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindSampler(0, 0);
  };

  auto show_cubemap = [this](int shadow_index, GLuint tbo) {
    sampler_point_shadow_.Bind(0);
    glBindTextureUnit(0, tbo);
    prog_.draw_cubemap.Use();
    prog_.draw_cubemap.SetUniform("uShadowIndex", shadow_index);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindSampler(0, 0);
  };

  auto show_light_grid = [this](float light_grid_divider) {
    fb_.prepass_linear_depth.Bind(0);
    prog_.draw_light_grid.Use();
    prog_.draw_light_grid.SetUniform("uLightGridDivider", light_grid_divider);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  };

  using enum opt::Layers::Enum;
  switch (opt::layers.show_layer) {
    case kBrdfLut:
      show_layer(Format::kRgb, fb_.brdf_lut);
      break;
    case kShowCascade:
      show_layer_csm(opt::layers.shadow_cascade, fb_.shadowmap_direct);
      break;
    case kPointShadow:
      show_cubemap(opt::layers.point_shadow, fb_.shadowmap_point);
      break;
    case kLightGrid:
      show_light_grid(opt::layers.light_grid_divider);
      break;
    case kPrepassLinearDepth:
      show_layer(Format::kDepthPrepass, fb_.prepass_linear_depth);
      break;
    case kPrepassAlphaBlendLinearDepth:
      show_layer(Format::kDepthPrepass, fb_.prepass_ab_linear_depth);
      break;
    case kOutlineNormals:
      show_layer(Format::kRgb, fb_.outline_normals_depth);
      break;
    case kOutlineDepth:
      show_layer(Format::kDepthOutline, fb_.outline_normals_depth);
      break;
    case kColor:
      show_layer(Format::kRgb, fb_.gbuffer_color);
      break;
    case kNormal:
      show_layer(Format::kRgb, fb_.gbuffer_normals);
      break;
    case kPbrRed:
      show_layer(Format::kRed, fb_.gbuffer_pbr);
      break;
    case kPbrGreen:
      show_layer(Format::kGreen, fb_.gbuffer_pbr);
      break;
    case kTransparencyAccum:
      show_layer(Format::kRgb, fb_.color_accum);
      break;
    case kTransparencyRevealage:
      show_layer(Format::kRed, fb_.revealage);
      break;
    case kHbao:
      show_layer(Format::kRed, fb_.hbao_blur_vertical_final);
      break;
    case kGtaoNormals:
      show_uint_layer(Format::kRgb, fb_.gtao_denoise_final);
      break;
    case kGtaoEdges:
      show_layer(Format::kRed, fb_.gtao_edges);
      break;
    case kGtaoVisibility:
      show_uint_layer(Format::kAlpha, fb_.gtao_denoise_final);
      break;
    case kFxaaLuma:
      show_layer(Format::kAlpha, fb_.ldr_luma);
      break;
    case kHdr:
      show_layer_mipmap(Format::kRgbHdr, opt::layers.bloom_mip_map,
                        fb_.hdr_map);
      break;
    default:
      break;
  }
}

void Renderer::DebugNormals() {
  // last Renderpass is View, no need to update indirect buffer
  RenderOpaqueAndAlphaClip(GL_BACK, prog_.debug_normals, vao_.viewpass);
}

void Renderer::DebugAxis() {
  prog_.draw_axis.Use();
  glDrawArrays(GL_LINES, 0, 12);
}

void Renderer::DebugPointLights() {
  prog_.debug_point_lights.Use();
  prog_.debug_point_lights.SetUniform("uAlpha", opt::debug.point_lights_alpha);

  gen_mesh_.DrawInternalMeshInstanced(GenMeshInternal::kPointLight,
                                      GL_TRIANGLES,
                                      atomic_counters_->frustum_point_lights);
}

void Renderer::ShowPointLight() {
  prog_.show_point_light.Use();
  prog_.show_point_light.SetUniform("uAlpha", opt::debug.point_lights_alpha);

  gen_mesh_.DrawInternalMesh(GenMeshInternal::kPointLight, GL_TRIANGLES);
}

void Renderer::DebugAABB() {
  prog_.debug_box_aabb.Use();
  prog_.debug_box_aabb.SetUniform("uColor", global::kGreen);
  // use vertex attribute buffers
  const auto& addr = gen_mesh_.GetVertexAddr(GenMeshInternal::kDebugBox);
  vao_.genmesh_pos_inst.Bind();
  addr.DrawInstancedBaseInstance(GL_LINES,
                                 atomic_counters_->frustum_instances_view, 0);
}

void Renderer::DebugOBB() {
  prog_.debug_box_obb.Use();
  prog_.debug_box_obb.SetUniform("uColor", global::kPurple);
  // use vertex attribute buffers
  const auto& addr = gen_mesh_.GetVertexAddr(GenMeshInternal::kDebugBox);
  vao_.genmesh_pos_inst.Bind();
  addr.DrawInstancedBaseInstance(GL_LINES,
                                 atomic_counters_->frustum_instances_view, 0);
}

void Renderer::DebugCameras() {
  prog_.debug_cameras.Use();
  prog_.debug_cameras.SetUniform("uColor", global::kYellow);
  cameras_.DebugCameras();
  cameras_.DrawCameras();
}

void Renderer::DebugParticles() {
  prog_.debug_particles.Use();
  prog_.debug_particles.SetUniform("uColor", global::kWhite);

  gen_mesh_.DrawInternalMeshInstanced(GenMeshInternal::kDebugBox, GL_LINES,
                                      particles_.fx_instance_count_);
}

void Renderer::GetDebugReadback() { sync_.FenceSt(); }

void Renderer::RenderLoading() {
  BindDefaultFramebuffer();

  // dummy VAO
  glBindVertexArray(gen_mesh_.pos_);

  prog_.loading_screen.Use();
  prog_.loading_screen.SetUniform("uTime", app::glfw.time);
  prog_.loading_screen.SetUniform("uResolution", app::glfw.window_size);
  prog_.loading_screen.SetUniform("uMouse", input::GetMousePos());
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::RenderScene() {
  using enum Metrics::Enum;
  ProcessMetrics();

  // scene (models, objects, lights, etc.)
  scene_.ProcessScene();
  // UBO for Renderer
  ubo_.UploadBuffers();
  // after UBO (compute shader)
  if (assets_.models_.build_indirect_cmd_) {
    BuildIndirectCmd();
    assets_.models_.build_indirect_cmd_ = false;
  }

  // start the frame, reset main counters
  atomic_counters_.ClearSubData(0, gpu::kCmdCountersSize);
  scene_.pick_entity_.ClearData();

  // point light culling
  if (opt::pipeline.point_lights) {
    queries_[kFrustumCullingPointLights].Begin();
    FrustumCullingPointLights();
    queries_[kFrustumCullingPointLights].End();

    queries_[kClusterLightCulling].Begin();
    ClusterLightCulling();
    queries_[kClusterLightCulling].End();
  } else {
    queries_[kFrustumCullingPointLights].Begin();
    DisablePointLights();
    queries_[kFrustumCullingPointLights].End();
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_cmd_draw_);

  // directional shadows use an orthographic projection,
  // distribution of depth is linear (no Reversed-Z)
  if (opt::pipeline.direct_shadowmaps) {
    queries_[kFrustumCullingDirectShadows].Begin();
    FrustumCulling(RenderPass::kDirectShadowPass);
    queries_[kFrustumCullingDirectShadows].End();

    queries_[kDirectShadowmap].Begin();
    DirectShadows();
    queries_[kDirectShadowmap].End();
  } else {
    queries_[kDirectShadowmap].Begin();
    DisableDirectShadows();
    queries_[kDirectShadowmap].End();
  }

  // point shadows use distance from fragment to light (no Reversed-Z)
  if (opt::pipeline.point_shadowmaps) {
    queries_[kFrustumCullingPointShadows].Begin();
    FrustumCulling(RenderPass::kPointShadowPass);
    queries_[kFrustumCullingPointShadows].End();

    queries_[kPointShadowmap].Begin();
    PointShadows();
    queries_[kPointShadowmap].End();
  } else {
    queries_[kPointShadowmap].Begin();
    DisablePointShadows();
    queries_[kPointShadowmap].End();
  }

  // enable Reversed-Z
  glDepthFunc(GL_GEQUAL);

  queries_[kFrustumCullingView].Begin();
  FrustumCulling(RenderPass::kViewPass);
  queries_[kFrustumCullingView].End();

  // separate pass for transparent objects
  queries_[kPrepassAlphaBlend].Begin();
  PrepassAlphaBlend();
  queries_[kPrepassAlphaBlend].End();

  // opaque overlap transparent (render occlusion texture)
  queries_[kPrepass].Begin();
  Prepass();
  queries_[kPrepass].End();

  // transparency part (WBOIT)
  if (opt::pipeline.alpha_blend) {
    // pick transparent object
    queries_[kAlphaBlendTexture].Begin();
    OcclusionAlphaBlendTexture();
    queries_[kAlphaBlendTexture].End();

    BindBlendFramebuffer();

    glEnable(GL_BLEND);
    glBlendFunci(0, GL_ONE, GL_ONE);
    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    glDepthMask(GL_FALSE);

    queries_[kAlphaBlend].Begin();
    ForwardAlphaBlend();
    queries_[kAlphaBlend].End();

    if (opt::pipeline.particles) {
      queries_[kParticles].Begin();
      ParticlesUpdate();
      ParticlesDraw();
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_cmd_draw_);
      queries_[kParticles].End();
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDisable(GL_BLEND);
  }

  // clear visibility buffer and populate from texture
  queries_[kOcclusionTexture].Begin();
  OcclusionTexture();
  queries_[kOcclusionTexture].End();

  queries_[kFrustumCullingViewOcclusion].Begin();
  FrustumCulling(RenderPass::kViewOcclusionPass);
  queries_[kFrustumCullingViewOcclusion].End();

  // pick object and light at crosshair,
  // gather renderpasses data (ui, debug)
  sync_.FenceSt();
  scene_.PickEntity();
  scene_.objects_.ReadVisibility();

  // after all visibility tests
  queries_[kOutlineSelectedTexture].Begin();
  OutlineSelectedTexture();
  queries_[kOutlineSelectedTexture].End();

  // opaque part (final shading)
  if (opt::pipeline.toggle_deferred_forward) {
    queries_[kGBuffer].Begin();
    GBuffer();
    queries_[kGBuffer].End();
  }

  glDepthFunc(GL_LEQUAL);
  glDisable(GL_DEPTH_TEST);

  // ambient occlusion
  if (opt::pipeline.hbao) {
    queries_[kHbao].Begin();
    RenderHbao();
    queries_[kHbao].End();
  }

  if (opt::pipeline.gtao) {
    queries_[kGtao].Begin();
    RenderGtao();
    queries_[kGtao].End();
  }

  BindHdrFramebuffer();

  queries_[kLighting].Begin();
  if (opt::pipeline.toggle_deferred_forward) {
    if (opt::pipeline.toggle_lighting_shadows) {
      DeferredLighting();
    } else {
      ShowShadowsDeferred();
    }
  } else {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    if (opt::pipeline.toggle_lighting_shadows) {
      ForwardLighting();
    } else {
      ShowShadowsForward();
    }
    glDisable(GL_DEPTH_TEST);
  }
  queries_[kLighting].End();

  if (opt::pipeline.skybox) {
    queries_[kSkybox].Begin();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    Skybox();
    glDisable(GL_DEPTH_TEST);
    queries_[kSkybox].End();
  }

  if (opt::pipeline.alpha_blend) {
    queries_[kCompositePassOIT].Begin();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
    CompositePassOIT();
    glDisable(GL_BLEND);
    queries_[kCompositePassOIT].End();
  }

  queries_[kOutlineSelectedDraw].Begin();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  OutlineSelectedDraw();
  glDisable(GL_BLEND);
  queries_[kOutlineSelectedDraw].End();

  const auto& debug = opt::debug;
  if (debug.flags != 0) {
    queries_[kDebug].Begin();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    using enum opt::DebugFlags::Enum;
    if (debug.flags & kNormals) {
      DebugNormals();
    }
    if (debug.flags & kAxis) {
      glDisable(GL_DEPTH_TEST);
      DebugAxis();
      glEnable(GL_DEPTH_TEST);
    }
    if (debug.flags & kLights) {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      DebugPointLights();
      ShowPointLight();
      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
    }
    if (debug.flags & kAABB) {
      DebugAABB();
    }
    if (debug.flags & kOBB) {
      DebugOBB();
    }
    if (debug.flags & kCameras) {
      DebugCameras();
    }
    if (debug.flags & kParticles) {
      DebugParticles();
    }
    glDisable(GL_DEPTH_TEST);
    queries_[kDebug].End();
  }

  BindLdrFramebuffer();

  if (opt::pipeline.bloom) {
    queries_[kBloom].Begin();
    Bloom();
    queries_[kBloom].End();
  }

  // HDR -> LDR -> sRGB
  queries_[kPostprocess].Begin();
  Postprocess();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  CrosshairOverlay();
  glDisable(GL_BLEND);
  queries_[kPostprocess].End();

  queries_[kFxaa].Begin();
  BindDefaultFramebuffer();
  DefaultFramebuffer();
  queries_[kFxaa].End();

  // debug
  ViewLayers();
}