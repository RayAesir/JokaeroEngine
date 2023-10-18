#pragma once

// local
#include "global.h"
#include "opengl/program.h"

// how to render the skinned and static geometry with alpha:
// 1. change shaders several times (switch cost)
// 2. uniform condition variable (all uniform buffers are bounded)
// 3. subroutines (performance cost is higher than several conditions)
// paths are relative to shaders folder
struct Programs {
  gl::Program loading_screen{"vert/screen_triangle.vert",
                             "frag/loading_screen.frag"};
  // frustum culling lights
  gl::Program frustum_culling_point_lights{
      "comp/frustum_culling_point_lights.comp"};
  gl::Program pick_point_light{"comp/pick_point_light.comp"};
  gl::Program sort_point_shadowmaps{"comp/sort_point_shadowmaps.comp"};
  gl::Program create_point_shadowmaps{"comp/create_point_shadowmaps.comp"};
  // frustum culling
  gl::Program cmd_build{"comp/cmd_build.comp"};
  gl::Program cmd_reset_instance_count{"comp/cmd_reset_instance_count.comp"};
  gl::Program cmd_set_base_instance{"comp/cmd_set_base_instance.comp"};
  gl::Program frustum_culling_direct_shadows{
      "comp/frustum_culling_direct_shadows.comp"};
  gl::Program frustum_culling_point_shadows{
      "comp/frustum_culling_point_shadows.comp"};
  gl::Program frustum_culling_view{"comp/frustum_culling_view.comp"};
  // stream compaction
  gl::Program instance_compact_direct_shadows{"comp/instance_compact.comp",
                                              "#define DIRECT_SHADOWS\n"};
  gl::Program instance_compact_point_shadows{"comp/instance_compact.comp",
                                             "#define POINT_SHADOWS\n"};
  gl::Program instance_compact_view{"comp/instance_compact.comp",
                                    "#define VIEW\n"};
  // image-based-lighting
  gl::Program equirectangular_to_cubemap{
      "vert/ibl_cubemap.vert", "geom/ibl_cubemap.geom",
      "frag/ibl_equirectangular_to_cubemap.frag"};
  gl::Program irradiance_convolution{"vert/ibl_cubemap.vert",
                                     "geom/ibl_cubemap.geom",
                                     "frag/ibl_irradiance_convolution.frag"};
  gl::Program prefilter_environment{"vert/ibl_cubemap.vert",
                                    "geom/ibl_cubemap.geom",
                                    "frag/ibl_prefilter_environment.frag"};
  gl::Program brdf_lut{"vert/screen_triangle.vert", "frag/ibl_brdf_lut.frag"};
  // shadows
  gl::Program shadows_dir_st_op{"vert/shadows_direct.vert",
                                "frag/shadows_direct.frag"};
  gl::Program shadows_dir_sk_op{shadows_dir_st_op, "#define SKINNED\n"};
  gl::Program shadows_dir_st_ac{shadows_dir_st_op, "#define ALPHA\n"};
  gl::Program shadows_dir_sk_ac{shadows_dir_st_op,
                                "#define SKINNED\n#define ALPHA\n"};

  gl::Program shadows_point_st_op{"vert/shadows_point.vert",
                                  "frag/shadows_point.frag"};
  gl::Program shadows_point_sk_op{shadows_point_st_op, "#define SKINNED\n"};
  gl::Program shadows_point_st_ac{shadows_point_st_op, "#define ALPHA\n"};
  gl::Program shadows_point_sk_ac{shadows_point_st_op,
                                  "#define SKINNED\n#define ALPHA\n"};
  // depth/normals prepass and occlusion
  gl::Program prepass_st_op{"vert/prepass.vert", "frag/prepass.frag"};
  gl::Program prepass_sk_op{prepass_st_op, "#define SKINNED\n"};
  gl::Program prepass_st_ac{prepass_st_op, "#define ALPHA\n"};
  gl::Program prepass_sk_ac{prepass_st_op, "#define SKINNED\n#define ALPHA\n"};
  gl::Program instance_reset_visibility{"comp/instance_reset_visibility.comp"};
  gl::Program occlusion_culling_texture{"comp/occlusion_culling_texture.comp"};
  gl::Program occlusion_culling_view{"comp/occlusion_culling_view.comp"};
  // full transparent objects prepass
  gl::Program occlusion_alpha_blend_texture{
      "comp/occlusion_alpha_blend_texture.comp"};
  // light culling, cluster shading
  gl::Program create_frustum_clusters{"comp/create_frustum_clusters.comp"};
  gl::Program cluster_light_culling{"comp/cluster_light_culling.comp"};
  // deferred opaque/alpha clip
  gl::Program gbuffer_st_op{"vert/gbuffer.vert", "frag/gbuffer.frag"};
  gl::Program gbuffer_sk_op{gbuffer_st_op, "#define SKINNED\n"};
  gl::Program gbuffer_st_ac{gbuffer_st_op, "#define ALPHA\n"};
  gl::Program gbuffer_sk_ac{gbuffer_st_op, "#define SKINNED\n#define ALPHA\n"};
  gl::Program deferred_lighting{"vert/deferred_quad.vert",
                                "frag/deferred_lighting.frag"};
  // forward opaque/alpha clip
  gl::Program forward_st_op{"vert/forward.vert", "frag/forward_lighting.frag"};
  gl::Program forward_sk_op{forward_st_op, "#define SKINNED\n"};
  gl::Program forward_st_ac{forward_st_op, "#define ALPHA\n"};
  gl::Program forward_sk_ac{forward_st_op, "#define SKINNED\n#define ALPHA\n"};
  // forward transparency/translucency (WBOIT)
  gl::Program forward_blend{"vert/forward.vert", "frag/forward_wboit.frag"};
  gl::Program forward_blend_skinned{forward_blend, "#define SKINNED\n"};
  gl::Program composite_pass{"vert/screen_triangle.vert",
                             "frag/composite_pass.frag"};
  // particles
  gl::Program particles_update{"comp/particles_update.comp"};
  gl::Program particles_render{"vert/particles.vert", "frag/particles.frag"};
  // outline (select objects)
  gl::Program outline_selected_texture_st_op{
      "vert/outline_selected_texture.vert",
      "frag/outline_selected_texture.frag"};
  gl::Program outline_selected_texture_sk_op{outline_selected_texture_st_op,
                                             "#define SKINNED\n"};
  gl::Program outline_selected_texture_st_ac{outline_selected_texture_st_op,
                                             "#define ALPHA\n"};
  gl::Program outline_selected_texture_sk_ac{
      outline_selected_texture_st_op, "#define SKINNED\n#define ALPHA\n"};
  gl::Program outline_selected{"vert/outline_selected_quad.vert",
                               "frag/outline_selected.frag"};
  // HDR bloom
  gl::Program bloom_downscale{"comp/bloom_downscale.comp"};
  gl::Program bloom_upscale{"comp/bloom_upscale.comp"};
  // postprocess
  gl::Program postprocess{"vert/screen_triangle.vert",
                          "frag/post_processing.frag"};
  gl::Program crosshair{"vert/crosshair.vert", "geom/crosshair.geom",
                        "frag/crosshair.frag"};
  // default framebuffer, fxaa
  gl::Program default_framebuffer{"vert/screen_triangle.vert",
                                  "frag/default_framebuffer.frag"};
  gl::Program fxaa_quality{"vert/screen_triangle.vert",
                           "frag/fxaa_quality.frag"};
  gl::Program fxaa_performance{"vert/screen_triangle.vert",
                               "frag/fxaa_performance.frag"};
  // hbao
  gl::Program hbao_deinterleave{"vert/screen_triangle.vert",
                                "frag/hbao_deinterleave.frag"};
  gl::Program hbao{"vert/screen_triangle.vert", "geom/screen_triangle.geom",
                   "frag/hbao.frag"};
  gl::Program hbao_interleave{"vert/screen_triangle.vert",
                              "frag/hbao_interleave.frag"};
  gl::Program hbao_blur_vertical{"vert/screen_triangle.vert",
                                 "frag/hbao_blur.frag"};
  gl::Program hbao_blur_horizontal{hbao_blur_vertical, "#define HORIZONTAL\n"};
  // gtao
  gl::Program gtao{"vert/screen_triangle.vert", "frag/gtao.frag"};
  gl::Program gtao_denoise{"comp/gtao_denoise.comp"};
  // other
  gl::Program skybox{"vert/skybox.vert", "frag/skybox.frag"};
  gl::Program draw_axis{"vert/draw_axis.vert", "frag/draw_axis.frag"};
  gl::Program draw_texture{"vert/screen_triangle.vert",
                           "frag/draw_texture.frag"};
  gl::Program draw_uint_texture{"vert/screen_triangle.vert",
                                "frag/draw_uint_texture.frag"};
  gl::Program draw_texture_csm{"vert/screen_triangle.vert",
                               "frag/draw_texture_csm.frag"};
  gl::Program draw_cubemap{"vert/screen_triangle.vert",
                           "frag/draw_cubemap.frag"};
  gl::Program draw_light_grid{"vert/screen_triangle.vert",
                              "frag/draw_light_grid.frag"};
  // show shadows
  gl::Program show_shadows_deferred{"vert/deferred_quad.vert",
                                    "frag/show_shadows_deferred.frag"};
  gl::Program show_shadows_forward_st_op{"vert/forward.vert",
                                         "frag/show_shadows_forward.frag"};
  gl::Program show_shadows_forward_sk_op{show_shadows_forward_st_op,
                                         "#define SKINNED\n"};
  gl::Program show_shadows_forward_st_ac{show_shadows_forward_st_op,
                                         "#define ALPHA\n"};
  gl::Program show_shadows_forward_sk_ac{show_shadows_forward_st_op,
                                         "#define SKINNED\n#define ALPHA\n"};
  // debug
  gl::Program debug_box_aabb{"vert/debug_box.vert", "frag/debug_box.frag"};
  gl::Program debug_box_obb{debug_box_aabb, "#define OBB\n"};
  gl::Program debug_point_lights{"vert/debug_point_lights.vert",
                                 "frag/debug_point_lights.frag"};
  gl::Program show_point_light{"vert/show_point_light.vert",
                               "frag/show_point_light.frag"};
  gl::Program debug_normals_static{"vert/debug_normals.vert",
                                   "geom/debug_normals.geom",
                                   "frag/debug_normals.frag"};
  gl::Program debug_normals_skinned{debug_normals_static, "#define SKINNED\n"};
  gl::Program debug_cameras{"vert/debug_cameras.vert", "frag/debug_box.frag"};
  gl::Program debug_particles{"vert/debug_particles.vert",
                              "frag/debug_box.frag"};

  const gl::Program* frustum_culling[RenderPass::kTotal]{
      &frustum_culling_direct_shadows,  //
      &frustum_culling_point_shadows,   //
      &frustum_culling_view,            //
      &occlusion_culling_view           //
  };
  const gl::Program* instance_compact[RenderPass::kTotal]{
      &instance_compact_direct_shadows,  //
      &instance_compact_point_shadows,   //
      &instance_compact_view,            //
      &instance_compact_view             //
  };
  const gl::Program* shadows_dir[MeshType::kNoAlphaBlend]{
      &shadows_dir_st_op,  //
      &shadows_dir_sk_op,  //
      &shadows_dir_st_ac,  //
      &shadows_dir_sk_ac   //
  };
  const gl::Program* shadows_point[MeshType::kNoAlphaBlend]{
      &shadows_point_st_op,  //
      &shadows_point_sk_op,  //
      &shadows_point_st_ac,  //
      &shadows_point_sk_ac   //
  };
  const gl::Program* prepass[MeshType::kTotal]{
      &prepass_st_op,  //
      &prepass_sk_op,  //
      &prepass_st_ac,  //
      &prepass_sk_ac,  //
      &prepass_st_op,  //
      &prepass_sk_op   //
  };
  const gl::Program* gbuffer[MeshType::kNoAlphaBlend]{
      &gbuffer_st_op,  //
      &gbuffer_sk_op,  //
      &gbuffer_st_ac,  //
      &gbuffer_sk_ac   //
  };
  const gl::Program* forward[MeshType::kNoAlphaBlend]{
      &forward_st_op,  //
      &forward_sk_op,  //
      &forward_st_ac,  //
      &forward_sk_ac   //
  };
  const gl::Program* outline_selected_texture[MeshType::kTotal]{
      &outline_selected_texture_st_op,  //
      &outline_selected_texture_sk_op,  //
      &outline_selected_texture_st_ac,  //
      &outline_selected_texture_sk_ac,  //
      &outline_selected_texture_st_op,  //
      &outline_selected_texture_sk_op   //
  };
  const gl::Program* show_shadows_forward[MeshType::kNoAlphaBlend]{
      &show_shadows_forward_st_op,  //
      &show_shadows_forward_sk_op,  //
      &show_shadows_forward_st_ac,  //
      &show_shadows_forward_sk_ac   //
  };
  const gl::Program* debug_normals[MeshType::kNoAlphaBlend]{
      &debug_normals_static,   //
      &debug_normals_skinned,  //
      &debug_normals_static,   //
      &debug_normals_skinned   //
  };
};