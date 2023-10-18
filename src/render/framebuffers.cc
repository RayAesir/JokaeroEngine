#include "framebuffers.h"

// deps
#include <spdlog/spdlog.h>

#include <glm/common.hpp>
// global
#include <vector>
// local
#include "app/parameters.h"
#include "global.h"
#include "options.h"

GLsizei Framebuffers::BloomMipmapLevel() {
  // avoid float-point precision error
  glm::ivec2 mip_size_int = app::opengl.framebuffer_size;
  GLsizei level = 0;
  // group size + filter size
  constexpr GLfloat kTileSize = 8.0f + 2.0f;
  // get the base level parameters too
  for (int i = 0; i < global::kBloomDownscaleLimit; ++i) {
    glm::vec2 mip_size = mip_size_int;
    if (mip_size.x > kTileSize || mip_size.y > kTileSize) {
      bloom_mipmaps[i].mip_size = mip_size;
      bloom_mipmaps[i].texel_size = glm::vec2(1.0f) / mip_size;
      bloom_mipmaps[i].workgroups = glm::ceil(mip_size / glm::vec2(8.0f));
      mip_size_int /= 2;
      ++level;
    } else {
      break;
    }
  }
  bloom_mip_levels = level - 1;
  return level;
};

Framebuffers::Framebuffers() {
  InitImageBasedLighting();
  InitDirectShadows();
  InitPointShadows();
  InitDefault();
}

void Framebuffers::InitImageBasedLighting() {
  const auto &lighting = opt::lighting;
  // equirectangular hdr or jpg
  environment_map = gl::TextureCubeMap{};
  environment_map.SetStorage2D(lighting.prefilter_max_level, GL_RGB16F,
                               lighting.environment_map_size,
                               lighting.environment_map_size);
  environment_map.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  environment_map.SetWrap3D(GL_CLAMP_TO_EDGE);
  // custom resolution, create own depth buffer object
  environment_depth = gl::TextureCubeMap{};
  environment_depth.SetStorage2D(1, GL_DEPTH_COMPONENT24,
                                 lighting.environment_map_size,
                                 lighting.environment_map_size);
  environment_depth.SetFilter(GL_LINEAR, GL_LINEAR);
  environment_depth.SetWrap3D(GL_CLAMP_TO_EDGE);
  // attachment
  environment.AttachTexture(GL_COLOR_ATTACHMENT0, environment_map);
  environment.AttachTexture(GL_DEPTH_ATTACHMENT, environment_depth);
  environment.CheckStatus();

  // converts the environment cubemap into irradiance cubemap
  // that used in image based lighting (ambient for pbr)
  irradiance_map = gl::TextureCubeMap{};
  irradiance_map.SetStorage2D(1, GL_RGB16F, lighting.irradiance_map_size,
                              lighting.irradiance_map_size);
  irradiance_map.SetFilter(GL_LINEAR, GL_LINEAR);
  irradiance_map.SetWrap3D(GL_CLAMP_TO_EDGE);

  // custom resolution, create own depth buffer object
  irradiance_depth = gl::TextureCubeMap{};
  irradiance_depth.SetStorage2D(1, GL_DEPTH_COMPONENT24,
                                lighting.irradiance_map_size,
                                lighting.irradiance_map_size);
  irradiance_depth.SetFilter(GL_LINEAR, GL_LINEAR);
  irradiance_depth.SetWrap3D(GL_CLAMP_TO_EDGE);
  // attachment
  irradiance.AttachTexture(GL_COLOR_ATTACHMENT0, irradiance_map);
  irradiance.AttachTexture(GL_DEPTH_ATTACHMENT, irradiance_depth);
  irradiance.CheckStatus();

  // converts the environment cubemap into prefiltered cubemap
  // that used in image based lighting (specular for pbr)
  prefilter_map = gl::TextureCubeMap{};
  prefilter_map.SetStorage2D(lighting.prefilter_max_level, GL_RGB16F,
                             lighting.prefilter_map_size,
                             lighting.prefilter_map_size);
  prefilter_map.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  prefilter_map.SetWrap3D(GL_CLAMP_TO_EDGE);
  // custom resolution, create own depth buffer object
  prefilter_depth = gl::TextureCubeMap{};
  prefilter_depth.SetStorage2D(
      lighting.prefilter_max_level, GL_DEPTH_COMPONENT24,
      lighting.prefilter_map_size, lighting.prefilter_map_size);
  prefilter_depth.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  prefilter_depth.SetWrap3D(GL_CLAMP_TO_EDGE);
  // attachment
  prefilter.AttachTexture(GL_COLOR_ATTACHMENT0, prefilter_map);
  prefilter.AttachTexture(GL_DEPTH_ATTACHMENT, prefilter_depth);
  prefilter.CheckStatus();

  // converter
  brdf_lut = gl::Texture2D{};
  brdf_lut.SetStorage2D(1, GL_RG16F, lighting.brdf_lut_map_size,
                        lighting.brdf_lut_map_size);
  brdf_lut.SetFilter(GL_LINEAR, GL_LINEAR);
  brdf_lut.SetWrap2D(GL_CLAMP_TO_EDGE);
  // custom resolution, create own depth buffer object
  brdf_depth = gl::RenderBuffer{};
  brdf_depth.SetStorage(GL_DEPTH_COMPONENT24, lighting.brdf_lut_map_size,
                        lighting.brdf_lut_map_size);
  // attachment
  brdf.AttachTexture(GL_COLOR_ATTACHMENT0, brdf_lut);
  brdf.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, brdf_depth);
  brdf.CheckStatus();
}

void Framebuffers::InitDirectShadows() {
  int shadowmap_size = opt::shadows.direct_shadowmap.Selected().size;

  // PCF require GL_LINEAR, range is [0, 1]
  shadowmap_direct = gl::Texture2DArray{};
  shadowmap_direct.SetStorage3D(1, GL_DEPTH_COMPONENT32F, shadowmap_size,
                                shadowmap_size, global::kMaxShadowCascades);
  shadowmap_direct.SetFilter(GL_LINEAR, GL_LINEAR);
  shadowmap_direct.SetWrap2D(GL_CLAMP_TO_BORDER);
  // the first component of GL_TEXTURE_BORDER_COLOR
  // is interpreted as a depth value, default vec4(0.0f)
  static const glm::vec4 border{1.0f, 0.0f, 0.0f, 0.0f};
  shadowmap_direct.SetBorderColor(border);
  shadowmap_direct.EnableCompareMode(GL_LEQUAL);
  // attachment
  shadows_direct.AttachTexture(GL_DEPTH_ATTACHMENT, shadowmap_direct);
  // explicitly tell OpenGL not going to render any color data
  shadows_direct.SetDrawBuffer(GL_NONE);
  shadows_direct.SetReadBuffer(GL_NONE);
  shadows_direct.CheckStatus();
}

void Framebuffers::InitPointShadows() {
  int shadowmap_size = opt::shadows.point_shadowmap.Selected().size;

  // PCF require GL_LINEAR, range is [0, 1]
  shadowmap_point = gl::TextureCubeMapArray{};
  // cubemaps array use layer-faces, 6 * depth
  shadowmap_point.SetStorage3D(1, GL_DEPTH_COMPONENT32F, shadowmap_size,
                               shadowmap_size, 6 * global::kMaxPointShadows);
  shadowmap_point.SetFilter(GL_LINEAR, GL_LINEAR);
  shadowmap_point.SetWrap3D(GL_CLAMP_TO_EDGE);
  shadowmap_point.EnableCompareMode(GL_LEQUAL);
  // attachment
  shadows_point.AttachTexture(GL_DEPTH_ATTACHMENT, shadowmap_point);
  // explicitly tell OpenGL not going to render any color data
  shadows_point.SetDrawBuffer(GL_NONE);
  shadows_point.SetReadBuffer(GL_NONE);
  shadows_point.CheckStatus();
}

void Framebuffers::InitDefault() {
  scene_depth = gl::RenderBuffer{};
  scene_depth.SetStorage(GL_DEPTH_COMPONENT32F, app::opengl.framebuffer_size);

  // linear depth
  prepass_linear_depth = gl::Texture2D{};
  prepass_linear_depth.SetStorage2D(1, GL_R32F, app::opengl.framebuffer_size);
  prepass_linear_depth.SetFilter(GL_NEAREST, GL_NEAREST);
  prepass_linear_depth.SetWrap2D(GL_CLAMP_TO_EDGE);
  // instance index (uint) cleaned with zeroes (dummy none)
  prepass_occlusion = gl::Texture2D{};
  prepass_occlusion.SetStorage2D(1, GL_R32UI, app::opengl.framebuffer_size);
  prepass_occlusion.SetFilter(GL_NEAREST, GL_NEAREST);
  prepass_occlusion.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  prepass.AttachTexture(GL_COLOR_ATTACHMENT0, prepass_linear_depth);
  prepass.AttachTexture(GL_COLOR_ATTACHMENT1, prepass_occlusion);
  prepass.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, scene_depth);
  // MRT
  prepass.SetDrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  prepass.CheckStatus();

  // linear depth
  prepass_ab_linear_depth = gl::Texture2D{};
  prepass_ab_linear_depth.SetStorage2D(1, GL_R32F,
                                       app::opengl.framebuffer_size);
  prepass_ab_linear_depth.SetFilter(GL_NEAREST, GL_NEAREST);
  prepass_ab_linear_depth.SetWrap2D(GL_CLAMP_TO_EDGE);
  // instance index (uint) cleaned with zeroes (dummy none)
  prepass_ab_occlusion = gl::Texture2D{};
  prepass_ab_occlusion.SetStorage2D(1, GL_R32UI, app::opengl.framebuffer_size);
  prepass_ab_occlusion.SetFilter(GL_NEAREST, GL_NEAREST);
  prepass_ab_occlusion.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  prepass_alpha_blend.AttachTexture(GL_COLOR_ATTACHMENT0,
                                    prepass_ab_linear_depth);
  prepass_alpha_blend.AttachTexture(GL_COLOR_ATTACHMENT1, prepass_ab_occlusion);
  prepass_alpha_blend.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, scene_depth);
  // MRT
  prepass_alpha_blend.SetDrawBuffers(
      {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  prepass_alpha_blend.CheckStatus();

  // 96 bit total
  // 0 - world space normals
  // https://aras-p.info/texts/CompactNormalStorage.html#method04spheremap
  // spheremap transform is designed for view space and
  // in world space sometimes produces artifacts
  // e.g. NdotV look like black dots but it's practically not visible
  gbuffer_normals = gl::Texture2D{};
  gbuffer_normals.SetStorage2D(1, GL_RG16F, app::opengl.framebuffer_size);
  gbuffer_normals.SetFilter(GL_NEAREST, GL_NEAREST);
  gbuffer_normals.SetWrap2D(GL_CLAMP_TO_EDGE);
  // 1 - color for opaque models
  gbuffer_color = gl::Texture2D{};
  gbuffer_color.SetStorage2D(1, GL_RGBA8, app::opengl.framebuffer_size);
  gbuffer_color.SetFilter(GL_NEAREST, GL_NEAREST);
  gbuffer_color.SetWrap2D(GL_CLAMP_TO_EDGE);
  // 2 - pbr: R - roughness, G - metalness
  gbuffer_pbr = gl::Texture2D{};
  gbuffer_pbr.SetStorage2D(1, GL_RG8, app::opengl.framebuffer_size);
  gbuffer_pbr.SetFilter(GL_NEAREST, GL_NEAREST);
  gbuffer_pbr.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  gbuffer.AttachTexture(GL_COLOR_ATTACHMENT0, gbuffer_normals);
  gbuffer.AttachTexture(GL_COLOR_ATTACHMENT1, gbuffer_color);
  gbuffer.AttachTexture(GL_COLOR_ATTACHMENT2, gbuffer_pbr);
  gbuffer.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, scene_depth);
  // MRT
  gbuffer.SetDrawBuffers(
      {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2});
  gbuffer.CheckStatus();

  // WBOIT
  // 0 - HDR accumulator
  color_accum = gl::Texture2D{};
  color_accum.SetStorage2D(1, GL_RGBA16F, app::opengl.framebuffer_size);
  color_accum.SetFilter(GL_LINEAR, GL_LINEAR);
  color_accum.SetWrap2D(GL_CLAMP_TO_EDGE);
  // 1 - revealage
  revealage = gl::Texture2D{};
  revealage.SetStorage2D(1, GL_R16F, app::opengl.framebuffer_size);
  revealage.SetFilter(GL_LINEAR, GL_LINEAR);
  revealage.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  transparency.AttachTexture(GL_COLOR_ATTACHMENT0, color_accum);
  transparency.AttachTexture(GL_COLOR_ATTACHMENT1, revealage);
  transparency.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, scene_depth);
  // MRT
  transparency.SetDrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  transparency.CheckStatus();

  // interleaved depth array 16 layers, quater sized
  hbao_depth_array = gl::Texture2DArray{};
  hbao_depth_array.SetStorage3D(1, GL_R32F, opt::hbao.quater_size,
                                global::kRandomElements);
  hbao_depth_array.SetFilter(GL_NEAREST, GL_NEAREST);
  hbao_depth_array.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  for (int i = 0; i < app::gpu.max_drawbuffers; ++i) {
    hbao_deinterleave.AttachTextureLayer(GL_COLOR_ATTACHMENT0 + i,
                                         hbao_depth_array, 0, i);
  }
  // MRT
  MiVector<GLenum> drawbuffers(app::gpu.max_drawbuffers);
  for (int i = 0; i < app::gpu.max_drawbuffers; ++i) {
    drawbuffers[i] = GL_COLOR_ATTACHMENT0 + i;
  }
  hbao_deinterleave.SetDrawBuffers(drawbuffers);
  hbao_deinterleave.CheckStatus();

  hbao_result_array = gl::Texture2DArray{};
  hbao_result_array.SetStorage3D(1, GL_RG16F, opt::hbao.quater_size,
                                 global::kRandomElements);
  hbao_result_array.SetFilter(GL_NEAREST, GL_NEAREST);
  hbao_result_array.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  hbao_compute.AttachTexture(GL_COLOR_ATTACHMENT0, hbao_result_array);
  hbao_compute.CheckStatus();

  hbao_interleaved = gl::Texture2D{};
  hbao_interleaved.SetStorage2D(1, GL_RG16F, app::opengl.framebuffer_size);
  hbao_interleaved.SetFilter(GL_NEAREST, GL_NEAREST);
  hbao_interleaved.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  hbao_interleave.AttachTexture(GL_COLOR_ATTACHMENT0, hbao_interleaved);
  hbao_interleave.CheckStatus();

  // blur, red - ao, green - depth
  hbao_blur_horizontal = gl::Texture2D{};
  hbao_blur_horizontal.SetStorage2D(1, GL_RG16F, app::opengl.framebuffer_size);
  hbao_blur_horizontal.SetFilter(GL_NEAREST, GL_NEAREST);
  hbao_blur_horizontal.SetWrap2D(GL_CLAMP_TO_EDGE);

  hbao_blur_vertical_final = gl::Texture2D{};
  hbao_blur_vertical_final.SetStorage2D(1, GL_RG16F,
                                        app::opengl.framebuffer_size);
  hbao_blur_vertical_final.SetFilter(GL_NEAREST, GL_NEAREST);
  hbao_blur_vertical_final.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  hbao_blur.AttachTexture(GL_COLOR_ATTACHMENT0, hbao_blur_horizontal);
  hbao_blur.AttachTexture(GL_COLOR_ATTACHMENT1, hbao_blur_vertical_final);
  hbao_blur.CheckStatus();

  // normals and visibility, packed for textureGather()
  gtao_normals_visibility = gl::Texture2D{};
  gtao_normals_visibility.SetStorage2D(1, GL_R32UI,
                                       app::opengl.framebuffer_size);
  gtao_normals_visibility.SetFilter(GL_NEAREST, GL_NEAREST);
  gtao_normals_visibility.SetWrap2D(GL_CLAMP_TO_EDGE);
  // edges for denoise
  gtao_edges = gl::Texture2D{};
  gtao_edges.SetStorage2D(1, GL_R8, app::opengl.framebuffer_size);
  gtao_edges.SetFilter(GL_NEAREST, GL_NEAREST);
  gtao_edges.SetWrap2D(GL_CLAMP_TO_EDGE);
  // first
  gtao_denoise = gl::Texture2D{};
  gtao_denoise.SetStorage2D(1, GL_R32UI, app::opengl.framebuffer_size);
  gtao_denoise.SetFilter(GL_NEAREST, GL_NEAREST);
  gtao_denoise.SetWrap2D(GL_CLAMP_TO_EDGE);
  // last
  gtao_denoise_final = gl::Texture2D{};
  gtao_denoise_final.SetStorage2D(1, GL_R32UI, app::opengl.framebuffer_size);
  gtao_denoise_final.SetFilter(GL_NEAREST, GL_NEAREST);
  gtao_denoise_final.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  gtao.AttachTexture(GL_COLOR_ATTACHMENT0, gtao_normals_visibility);
  gtao.AttachTexture(GL_COLOR_ATTACHMENT1, gtao_edges);
  // MRT
  gtao.SetDrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  gtao.CheckStatus();

  // empty framebuffer, no attachments
  glNamedFramebufferParameteri(empty, GL_FRAMEBUFFER_DEFAULT_WIDTH,
                               app::opengl.framebuffer_size.x);
  glNamedFramebufferParameteri(empty, GL_FRAMEBUFFER_DEFAULT_HEIGHT,
                               app::opengl.framebuffer_size.y);
  glNamedFramebufferParameteri(empty, GL_FRAMEBUFFER_DEFAULT_LAYERS, 0);
  glNamedFramebufferParameteri(empty, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 0);
  empty.CheckStatus();

  // normals and depth for edge detection
  outline_normals_depth = gl::Texture2D{};
  outline_normals_depth.SetStorage2D(1, GL_RGBA16F,
                                     app::opengl.framebuffer_size);
  outline_normals_depth.SetFilter(GL_NEAREST, GL_NEAREST);
  outline_normals_depth.SetWrap2D(GL_CLAMP_TO_EDGE);
  // outline depth
  outline_depth = gl::RenderBuffer{};
  outline_depth.SetStorage(GL_DEPTH_COMPONENT32F, app::opengl.framebuffer_size);
  // attachment
  outline.AttachTexture(GL_COLOR_ATTACHMENT0, outline_normals_depth);
  outline.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, outline_depth);
  outline.CheckStatus();

  // HDR, mipmaps for bloom
  GLsizei mipmap_level = BloomMipmapLevel();
  hdr_map = gl::Texture2D{};
  hdr_map.SetStorage2D(mipmap_level, GL_RGBA16F, app::opengl.framebuffer_size);
  hdr_map.SetFilter(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);
  hdr_map.SetWrap2D(GL_CLAMP_TO_EDGE);
  // default attachment or not complete
  hdr.AttachTexture(GL_COLOR_ATTACHMENT0, hdr_map);
  // skybox needs depth
  hdr.AttachRenderBuffer(GL_DEPTH_ATTACHMENT, scene_depth);
  hdr.CheckStatus();

  // LDR with Luma in alpha channel for FXAA
  // texture filtering should be enabled (GL_LINEAR)
  ldr_luma = gl::Texture2D{};
  ldr_luma.SetStorage2D(1, GL_RGBA8, app::opengl.framebuffer_size);
  ldr_luma.SetFilter(GL_LINEAR, GL_LINEAR);
  ldr_luma.SetWrap2D(GL_CLAMP_TO_EDGE);
  // attachment
  ldr.AttachTexture(GL_COLOR_ATTACHMENT0, ldr_luma);
  ldr.CheckStatus();
}
