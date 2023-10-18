#pragma once

// global
#include <array>
// local
#include "opengl/framebuffer.h"
#include "opengl/texture.h"

struct BloomMipmap {
  glm::ivec2 mip_size;
  glm::vec2 texel_size;
  glm::ivec2 workgroups;
};

struct Framebuffers {
  Framebuffers();

  gl::TextureCubeMap environment_map;
  gl::TextureCubeMap environment_depth;
  gl::FrameBuffer environment{"Environment"};

  gl::TextureCubeMap irradiance_map;
  gl::TextureCubeMap irradiance_depth;
  gl::FrameBuffer irradiance{"Irradiance"};

  gl::TextureCubeMap prefilter_map;
  gl::TextureCubeMap prefilter_depth;
  gl::FrameBuffer prefilter{"Prefilter"};

  gl::Texture2D brdf_lut;
  gl::RenderBuffer brdf_depth;
  gl::FrameBuffer brdf{"BRDF LUT"};

  gl::Texture2DArray shadowmap_direct;
  gl::FrameBuffer shadows_direct{"Shadows Direct"};

  gl::TextureCubeMapArray shadowmap_point;
  gl::FrameBuffer shadows_point{"Shadows Point"};

  gl::RenderBuffer scene_depth;

  gl::Texture2D prepass_linear_depth;
  gl::Texture2D prepass_occlusion;
  gl::FrameBuffer prepass{"Prepass"};

  gl::Texture2D prepass_ab_linear_depth;
  gl::Texture2D prepass_ab_occlusion;
  gl::FrameBuffer prepass_alpha_blend{"PrepassAlphaBlend"};

  gl::Texture2D gbuffer_normals;
  gl::Texture2D gbuffer_color;
  gl::Texture2D gbuffer_pbr;
  gl::FrameBuffer gbuffer{"GBuffer"};

  gl::Texture2D color_accum;
  gl::Texture2D revealage;
  gl::FrameBuffer transparency{"Transparency"};

  gl::Texture2DArray hbao_depth_array;
  gl::FrameBuffer hbao_deinterleave{"HBAO Deinterleave"};

  gl::Texture2DArray hbao_result_array;
  gl::FrameBuffer hbao_compute{"HBAO Compute"};

  gl::Texture2D hbao_interleaved;
  gl::FrameBuffer hbao_interleave{"HBAO Interleave"};

  gl::Texture2D hbao_blur_horizontal;
  gl::Texture2D hbao_blur_vertical_final;
  gl::FrameBuffer hbao_blur{"HBAO Blur"};

  // OpenGL adaptation of https://github.com/GameTechDev/XeGTAO
  gl::Texture2D gtao_normals_visibility;
  gl::Texture2D gtao_edges;
  gl::Texture2D gtao_denoise;
  gl::Texture2D gtao_denoise_final;
  gl::FrameBuffer gtao{"GTAO"};

  // bloom, GTAO denoise, anything with compute shaders
  gl::FrameBuffer empty{"Empty"};

  gl::Texture2D outline_normals_depth;
  gl::RenderBuffer outline_depth;
  gl::FrameBuffer outline{"Outline"};

  // ten levels should be enough for 4k
  std::array<BloomMipmap, 10> bloom_mipmaps;
  GLint bloom_mip_levels;
  GLsizei BloomMipmapLevel();
  gl::Texture2D hdr_map;
  gl::FrameBuffer hdr{"HDR"};

  gl::Texture2D ldr_luma;
  gl::FrameBuffer ldr{"LDR"};

  // act as reload too (RAII)
  void InitImageBasedLighting();
  void InitDirectShadows();
  void InitPointShadows();
  void InitDefault();
};