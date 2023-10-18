#include "options.h"

// local
#include "app/ini.h"
#include "app/parameters.h"

namespace opt {

namespace types {

Engine::Engine() { mouse_sensitivity = 0.04f; }

Textures::Textures() {
  anisotropy = {
      .label = "Anisotropy Filtering",  //
      .list =
          {
              {1.0, "None"},  //
              {2.0, "2x"},    //
              {4.0, "4x"},    //
              {8.0, "8x"},    //
              {16.0, "16x"},  //
          },
      .current = 4  //
  };

  lod_bias = {
      .label = "LOD Bias",  //
      .list =
          {
              {-1.0f, "-1.0"},  //
              {-0.5f, "-0.5"},  //
              {0.0f, "0.0"},    //
              {0.5f, "0.5"},    //
              {1.0f, "1.0"},    //
          },
      .current = 2  //
  };
}

Pipeline::Pipeline() {
  toggle_deferred_forward = true;
  toggle_lighting_shadows = true;
  point_lights = true;
  direct_shadowmaps = true;
  point_shadowmaps = true;
  alpha_blend = true;
  particles = false;
  hbao = true;
  gtao = false;
  skybox = true;
  bloom = true;
}

Lighting::Lighting() {
  // ibl
  show_irradiance = false;
  show_prefiltered = false;
  environment_map_size = 2048;
  irradiance_map_size = 128;
  prefilter_max_level = 5;
  prefilter_map_size = 512;
  brdf_lut_map_size = 512;
  cubemap_lod = 0.0f;
  // oit
  mesh_pow = 6.0f;
  mesh_clamp = 1000.0f;
  particles_pow = 0.3f;
  particles_clamp = 1000.0f;
}

Shadows::Shadows() {
  pcf_samples = {
      .label = "PCF Samples",  //
      .list =
          {
              {25, "25"},    //
              {32, "32"},    //
              {64, "64"},    //
              {100, "100"},  //
              {128, "128"},  //
          },
      .current = 2  //
  };

  direct_shadowmap = {
      .label = "Direct Shadowmap Size",  //
      .list =
          {
              {1024, "1024"},  //
              {2048, "2048"},  //
              {4096, "4096"},  //
          },
      .current = 1  //
  };

  num_cascades = 4;
  split_lambda = 0.7f;
  sample_offset = 100.0f;
  polygon_offset_factor = 1.0f;
  polygon_offset_units = 1024.0f;

  point_shadowmap = {
      .label = "Point Shadowmap Size",  //
      .list =
          {
              {256, "256"},    //
              {512, "512"},    //
              {1024, "1024"},  //
          },
      .current = 1  //
  };

  pcf_radius_uv = 0.04f;
  point_radius_uv = 0.1f;
}

ShowLayers::ShowLayers() {
  show_layer = 0;
  shadow_cascade = 0;
  point_shadow = 0;
  light_grid_divider = 32;
  bloom_mip_map = 0;
}

Outline::Outline() {
  depth_threshold = 1.0f;
  depth_normal_threshold = 0.5f;
  depth_normal_threshold_scale = 7.0f;
  normal_threshold = 0.8f;
  width = 1.0f;
  color = glm::vec3(0.9f, 0.9f, 0.9f);
}

Postprocess::Postprocess() {
  use_srgb_encoding = true;
  exposure = 1.5f;
  tonemapping = {
      .label = "Tonemapping Types",  //
      .list =
          {
              {opt::Tonemapping::kReinhard, "Reinhard"},            //
              {opt::Tonemapping::kReinhardLuma, "Reinhard Luma"},   //
              {opt::Tonemapping::kRomBinDaHouse, "RomBinDaHouse"},  //
              {opt::Tonemapping::kFilmic, "Filmic ACES"},           //
              {opt::Tonemapping::kUncharted2, "Uncharted2"},        //
              {opt::Tonemapping::kTotal, "Disable"},                //
          },
      .current = 1  //
  };
  // blur
  blur = false;
  blur_sigma = 1.0f;
  // bloom
  bloom_threshold = 1.15f;
  bloom_knee = 0.2f;
  bloom_intensity = 1.0f;
  // fxaa
  fxaa = true;
  fxaa_quality = true;
}

Hbao::Hbao() {
  radius = 2.0f;
  intensity = 1.0f;
  bias = 0.5f;
  blur_sharpness = 12.0f;
  quater_size = glm::ivec2(1920 / 4, 1080 / 4);
}

Gtao::Gtao() {
  presets = {
      .label = "GTAO Presets",  //
      .list =
          {
              {{.slice_count = 1.0f, .steps_per_slice = 2.0f}, "Low"},     //
              {{.slice_count = 2.0f, .steps_per_slice = 2.0f}, "Medium"},  //
              {{.slice_count = 3.0f, .steps_per_slice = 3.0f}, "High"},    //
              {{.slice_count = 9.0f, .steps_per_slice = 3.0f}, "Ultra"},   //
          },
      .current = 2  //
  };

  slice_count = 3.0f;
  steps_per_slice = 3.0f;
  effect_radius = 0.5f;
  effect_falloff_range = 0.615f;
  radius_multiplier = 1.457f;
  final_value_power = 2.2f;
  sample_distribution_power = 2.0f;
  thin_occluder_compensation = 0.2f;
  denoise_blur_beta = 1.2f;
  two_pass_blur = true;
}

Crosshair::Crosshair() {
  flags = CrosshairFlags::kCross;
  size = 0.05f;
  inner_size = 0.2f;
  thickness = 3.0f;
  color = glm::vec4(0.11f, 0.9f, 0.155f, 0.8f);
}

Debug::Debug() {
  normals_color = glm::vec3(1.0f, 1.0f, 0.0f);
  normals_magnitude = 0.5f;
  point_lights_alpha = 0.3f;
  flags = 0;
  switch1 = false;
  switch2 = false;
}

}  // namespace types

namespace set {

void UpdateScreenSize() {
  app::set::UpdateScreenSize();
  // integer
  hbao.quater_size = (app::opengl.framebuffer_size + 3) / 4;
}

ini::Description GetIniDescription() {
  ini::Description desc;
  desc.bools = {
      {"Pipeline", "bHBAO", &pipeline.hbao},
      {"Pipeline", "bGTAO", &pipeline.gtao},
      {"Postprocess", "bUseSrgbEncoding", &postprocess.use_srgb_encoding},
      {"Postprocess", "bFxaa", &postprocess.fxaa},
      {"Postprocess", "bFxaaQuality", &postprocess.fxaa_quality},
  };
  desc.ints = {
      {"Textures", "iMaxAnisotropy", &textures.anisotropy.current, 0, 4},
      {"Textures", "iLodBias", &textures.lod_bias.current, 0, 4},
      {"Postprocess", "iToneMappingType", &postprocess.tonemapping.current, 0,
       5},
  };
  desc.floats = {
      {"Engine", "fMouseSensitivity", &engine.mouse_sensitivity, 0.01f, 1.0f},
      {"Postprocess", "fExposure", &postprocess.exposure, 0.0f, 10.0f},
  };

  // append Application parameters
  ini::Description app_desc = app::set::GetIniDescription();
  desc.Append(app_desc);
  return desc;
}

}  // namespace set

}  // namespace opt