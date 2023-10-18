#include "win_settings.h"

// deps
#include <imgui.h>
// local
#include "app/ini.h"
#include "app/parameters.h"
#include "events.h"
#include "imgui_toggle/imgui_toggle.h"
#include "mem_info.h"
#include "options.h"
#include "render/renderer.h"
#include "ui/base_components.h"
#include "utils/enums.h"

namespace ui {

void WinSettings::ShowOpenGL() {
  auto& opengl = app::set::opengl;
  auto& engine = opt::set::engine;
  auto& textures = opt::set::textures;

  ImGui::Text("Vendor: %s", app::gpu.vendor);
  ImGui::Text("Renderer: %s", app::gpu.renderer);

  ImGui::Text("Monitor: %s", app::glfw.monitor_name.c_str());
  ImGui::Text("Current Resolution: [%d, %d]", opengl.framebuffer_size.x,
              opengl.framebuffer_size.y);
  ImGui::Text("Window Size: [%d, %d]", app::glfw.window_size.x,
              app::glfw.window_size.y);
  ImGui::Text("Display Resolution: [%d, %d]", app::glfw.videomode_size.x,
              app::glfw.videomode_size.y);
  ImGui::Text("DPI Scale: %.2f", app::glfw.window_scale.x);
  ImGui::Text("Refresh Rate: %d", app::glfw.refresh_rate);

  ImGui::Separator();

  ui::ShowComboBox(opengl.window_mode);
  ui::ShowComboBox(opengl.resolution);
  if (ImGui::Checkbox("Resizeable Window", &opengl.resizeable)) {
    event::Invoke(Event::kSetResizeableWindow);
  };
  if (ImGui::Button("Apply Screen Settings##opengl")) {
    opt::set::UpdateScreenSize();
    event::Invoke(Event::kSetWindowMode);
    event::Invoke(Event::kApplyResolution);
  }

  ImGui::Separator();

  ImGui::Text("Textures:");
  ui::ShowComboBox(textures.anisotropy);
  ui::ShowComboBox(textures.lod_bias);
  if (ImGui::Button("Apply Texture Settings##textures")) {
    event::Invoke(Event::kApplyTextureSettings);
  }

  ImGui::Separator();

  ImGui::Text("Input:");
  ImGui::SliderFloat("Mouse Sensitivity", &engine.mouse_sensitivity, 0.01f,
                     1.0f, "%.3f");

  ImGui::Separator();

  ImGui::Text("OpenGL Debug:");
  if (ImGui::Checkbox("Show Console Output", &opengl.show_debug)) {
    event::Invoke(Event::kToggleOpenGlDebug);
  };

  ImGui::Text("Message Severity:");
  for (auto& msg : opengl.msg_severity) {
    ImGui::Checkbox(msg.label, &msg.enable);
  }

  static int flags = ImGuiToggleFlags_Animated;
  if (ImGui::Button("Apply Severity##opengl")) {
    event::Invoke(Event::kApplyOpenGlMessageSeverity);
  }
}

void WinSettings::ShowPipeline() {
  auto& pipeline = opt::set::pipeline;
  static int flags = ImGuiToggleFlags_Animated;

  static constexpr const char* toggle_rendering[2]{"Forward+##pipe",
                                                   "Deferred##pipe"};
  static constexpr const char* toggle_lighting[2]{"Only Shadows##pipe",
                                                  "Full Lighting##pipe"};

  ImGui::Toggle(toggle_rendering[pipeline.toggle_deferred_forward],
                &pipeline.toggle_deferred_forward, flags);
  ImGui::SameLine();
  ImGui::Toggle(toggle_lighting[pipeline.toggle_lighting_shadows],
                &pipeline.toggle_lighting_shadows, flags);

  ImGui::Text("Enable:");
  ImGui::Checkbox("Point Lights##pipe", &pipeline.point_lights);

  ImGui::Checkbox("Direct Shadowmaps##pipe", &pipeline.direct_shadowmaps);
  ImGui::SameLine();
  ImGui::Checkbox("Point Shadowmaps##pipe", &pipeline.point_shadowmaps);

  ImGui::Checkbox("Transparency (WBOIT)##pipe", &pipeline.alpha_blend);
  ImGui::SameLine();
  ImGui::Checkbox("Particles##pipe", &pipeline.particles);

  if (ImGui::Checkbox("HBAO##pipe", &pipeline.hbao)) {
    pipeline.gtao = false;
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("GTAO##pipe", &pipeline.gtao)) {
    pipeline.hbao = false;
  }

  ImGui::Checkbox("Skybox##pipe", &pipeline.skybox);
  ImGui::SameLine();
  ImGui::Checkbox("Bloom##pipe", &pipeline.bloom);
}

void WinSettings::ShowLighting() {
  auto& lighting = opt::set::lighting;

  ImGui::Text("IBL:");
  ImGui::Text("Environment Size: [%d]", lighting.environment_map_size);
  ImGui::Text("Irradiance Size: [%d]", lighting.irradiance_map_size);
  ImGui::Text("Prefilter Size: [%d]", lighting.prefilter_map_size);
  ImGui::Text("BRDF LUT Size: [%d]", lighting.brdf_lut_map_size);
  ImGui::Checkbox("Show Irradiance##lighting", &lighting.show_irradiance);
  ImGui::SameLine();
  ImGui::Checkbox("Show Prefiltered##lighting", &lighting.show_prefiltered);
  ImGui::SliderFloat("Lod##shader", &lighting.cubemap_lod, 0.0f, 4.0f);
  if (ImGui::Button("Generate BRDF Lut##lighting")) {
    event::Invoke(Event::kGenerateBRDFLut);
  }

  ImGui::Separator();

  ImGui::Text("WBOIT:");
  ImGui::SliderFloat("Mesh Pow##lighting", &lighting.mesh_pow, 0.01f, 12.0f,
                     "%.2f");
  ImGui::SliderFloat("Mesh Clamp##lighting", &lighting.mesh_clamp, 0.01f,
                     3000.0f, "%.2f");
  ImGui::SliderFloat("Particles Pow##lighting", &lighting.particles_pow, 0.01f,
                     12.0f, "%.2f");
  ImGui::SliderFloat("Particles Clamp##lighting", &lighting.particles_clamp,
                     0.01f, 3000.0f, "%.2f");
}

void WinSettings::ShowShadows() {
  auto& shadows = opt::set::shadows;

  ImGui::Text("Common Settings:");
  if (ui::ShowComboBox(shadows.direct_shadowmap)) {
    event::Invoke(Event::kApplyDirShadowmapResolution);
  }
  if (ui::ShowComboBox(shadows.point_shadowmap)) {
    event::Invoke(Event::kApplyPointShadowmapResolution);
  }
  if (ui::ShowComboBox(shadows.pcf_samples)) {
    event::Invoke(Event::kUpdateShadowsPoisson);
  }

  ImGui::Separator();

  ImGui::Text("Direct Shadows PCF:");
  ImGui::SliderFloat("PCF Radius UV##shadows", &shadows.pcf_radius_uv, 0.001f,
                     0.1f, "%.3f");

  ui::SliderUint("Num Cascades##shadows", &shadows.num_cascades, 1, 4);
  ImGui::SliderFloat("Split Lambda##shadows", &shadows.split_lambda, 0.1f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Offset Factor##shadows", &shadows.polygon_offset_factor,
                     1.0f, 2.0f);
  ImGui::SliderFloat("Offset Units##shadows", &shadows.polygon_offset_units,
                     1024.0f, 4096.0f);

  ImGui::Separator();

  ImGui::Text("Point Shadows PCF:");
  ImGui::SliderFloat("Light PCF Radius UV##shadows", &shadows.point_radius_uv,
                     0.001f, 0.5f, "%.3f");

  ImGui::Separator();

  ImGui::Text("Cascades Info:");
  for (unsigned int i = 0; i < global::kMaxShadowCascades; ++i) {
    ImGui::Text("Frustum Center %d: [x: %.3f, y: %.3f, z: %.3f]", i,
                csm_.center[i].x, csm_.center[i].y, csm_.center[i].z);
  }
  ImGui::Text("Split: [0: %.3f] [1: %.3f] [2: %.3f] [3: %.3f]", csm_.split[0],
              csm_.split[1], csm_.split[2], csm_.split[3]);
  ImGui::Text("Radius: [0: %.3f] [1: %.3f] [2: %.3f] [3: %.3f]", csm_.radius[0],
              csm_.radius[1], csm_.radius[2], csm_.radius[3]);

  static constexpr const char* labels[global::kMaxShadowCascades]{
      "Cascade 1", "Cascade 2", "Cascade 3", "Cascade 4"};
  if (ImGui::CollapsingHeader("DOP##shadows")) {
    if (ImGui::BeginTabBar("Debug DOP##shadows")) {
      for (int c = 0; c < global::kMaxShadowCascades; ++c) {
        if (ImGui::BeginTabItem(labels[c])) {
          for (unsigned int p = 0; p < csm_.dop_total[c]; p++) {
            const auto& plane = csm_.dop_planes[c][p];
            ImGui::Text("%u: [%.2f, %.2f, %.2f], dist: [%.2f]", p, plane.x,
                        plane.y, plane.z, plane.w);
          }
          ImGui::EndTabItem();
        }
      }
      ImGui::EndTabBar();
    }
  }
}

void WinSettings::ShowAmbientOcclusion() {
  auto& pipeline = opt::set::pipeline;
  auto& hbao = opt::set::hbao;
  auto& gtao = opt::set::gtao;

  ImGui::Text("HBAO:");
  if (ImGui::Checkbox("Enable##hbao", &pipeline.hbao)) {
    pipeline.gtao = false;
  }
  ImGui::SliderFloat("Radius##hbao", &hbao.radius, 0.1f, 4.0f, "%.1f");
  ImGui::SliderFloat("Intensity##hbao", &hbao.intensity, 0.1f, 4.0f, "%.1f");
  ImGui::SliderFloat("Bias##hbao", &hbao.bias, 0.1f, 1.0f, "%.1f");

  ImGui::SliderFloat("Blur Sharpness##ao", &hbao.blur_sharpness, 0.1f, 12.0f,
                     "%.1f");

  ImGui::Separator();

  ImGui::Text("GTAO:");
  if (ImGui::Checkbox("Enable##gtao", &pipeline.gtao)) {
    pipeline.hbao = false;
  }
  if (ui::ShowComboBox(gtao.presets)) {
    gtao.slice_count = gtao.presets.Selected().slice_count;
    gtao.steps_per_slice = gtao.presets.Selected().steps_per_slice;
  }
  ImGui::SliderFloat("Radius##gtao", &gtao.effect_radius, 0.01f, 5.0f, "%.2f");
  ImGui::SliderFloat("Radius Multiplier##gtao", &gtao.radius_multiplier, 0.3f,
                     3.0f, "%.1f");
  ImGui::SliderFloat("Falloff Range##gtao", &gtao.effect_falloff_range, 0.0f,
                     1.0f, "%.1f");
  ImGui::SliderFloat("Sample Distribution Power##gtao",
                     &gtao.sample_distribution_power, 1.0f, 3.0f, "%.1f");
  ImGui::SliderFloat("Thin Occluder Compensation##gtao",
                     &gtao.thin_occluder_compensation, 0.0f, 0.7f, "%.1f");
  ImGui::SliderFloat("Final Power##gtao", &gtao.final_value_power, 0.5f, 5.0f,
                     "%.1f");
  ImGui::SliderFloat("Denoise Blur Beta##gtao", &gtao.denoise_blur_beta, 0.1f,
                     2.0f, "%.1f");
  ImGui::Checkbox("Two Pass Blur##gtao", &gtao.two_pass_blur);
}

void WinSettings::ShowPostprocess() {
  auto& postprocess = opt::set::postprocess;

  ImGui::Text("True = sRGB Luminosity Encoding (Correct)");
  ImGui::Text("False = Gamma 2.2 Encoding (Approximation)");
  ImGui::Checkbox("Encoding Type##pp", &postprocess.use_srgb_encoding);
  ImGui::SliderFloat("Exposure##pp", &postprocess.exposure, 0.1f, 4.0f, "%.1f");

  ui::ShowListBox(postprocess.tonemapping);

  ImGui::Separator();

  ImGui::Checkbox("Blur##pp", &postprocess.blur);
  ImGui::SliderFloat("Blur Sigma##pp", &postprocess.blur_sigma, 0.1f, 7.0f,
                     "%.1f");

  ImGui::Separator();

  ImGui::Checkbox("Bloom##pp", &opt::set::pipeline.bloom);
  ImGui::SliderFloat("Threshold##pp", &postprocess.bloom_threshold, 0.0f, 10.0f,
                     "%.2f");
  ImGui::SliderFloat("Knee##pp", &postprocess.bloom_knee, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat("Intensity##pp", &postprocess.bloom_intensity, 0.0f, 3.0f,
                     "%.2f");

  ImGui::Separator();

  ImGui::Checkbox("FXAA##pp", &postprocess.fxaa);
  ImGui::SameLine();
  ImGui::Checkbox("FXAA Quality##pp", &postprocess.fxaa_quality);
}

void WinSettings::ShowOutline() {
  auto& outline = opt::set::outline;
  ImGui::SliderFloat("Outline Width##outline", &outline.width, 0.1f, 2.0f,
                     "%.1f");
  ImGui::SliderFloat("Depth Threshold##outline", &outline.depth_threshold, 0.1f,
                     2.0f, "%.1f");
  ImGui::SliderFloat("Depth-Normal Threshold##outline",
                     &outline.depth_normal_threshold, 0.1f, 1.0f, "%.2f");
  ImGui::SliderFloat("Depth-Normal Threshold Scale##outline",
                     &outline.depth_normal_threshold_scale, 0.1f, 10.0f,
                     "%.1f");
  ImGui::SliderFloat("Normal Threshold##outline", &outline.normal_threshold,
                     0.1f, 1.0f, "%.2f");
  ImGui::ColorEdit3("Outline Color##outline", &outline.color[0]);
}

void WinSettings::ShowCrosshair() {
  using enum opt::CrosshairFlags::Enum;
  auto& crosshair = opt::set::crosshair;
  ImGui::CheckboxFlags("Square##crosshair", &crosshair.flags, kSquare);
  ImGui::SameLine();
  ImGui::CheckboxFlags("Cross##crosshair", &crosshair.flags, kCross);
  ImGui::SameLine();
  ImGui::CheckboxFlags("Circle##crosshair", &crosshair.flags, kCircle);

  ImGui::SliderFloat("Size##crosshair", &crosshair.size, 0.01f, 0.25f, "%.2f");
  ImGui::SliderFloat("Inner Size##crosshair", &crosshair.inner_size, 0.01f,
                     0.5f, "%.2f");
  ImGui::SliderFloat("Thickness##crosshair", &crosshair.thickness, 1.0f, 8.0f,
                     "%.1f");
  ImGui::ColorEdit4("Color##crosshair", &crosshair.color[0]);
}

void WinSettings::ShowLayers() {
  using enum opt::Layers::Enum;
  auto& layers = opt::set::layers;

  static bool buttons[kTotal]{};
  static int flags = ImGuiToggleFlags_Animated;

  ImGui::Text("IBL:");
  ImGui::Toggle("BRDF Lut##layer", &buttons[kBrdfLut], flags);

  ImGui::Text("CSM:");
  ImGui::Toggle("Show Cascade##layer", &buttons[kShowCascade], flags);
  ImGui::SliderInt("Cascade Number##layer", &layers.shadow_cascade, 0,
                   global::kMaxShadowCascades - 1);

  ImGui::Text("Point Shadows:");
  ImGui::Toggle("Show Cubemap##layer", &buttons[kPointShadow], flags);
  ImGui::SliderInt("Point Light Index##layer", &layers.point_shadow, 0,
                   global::kMaxPointShadows - 1);

  ImGui::Text("Frustum Clusters:");
  ImGui::Toggle("Light Count##layer", &buttons[kLightGrid], flags);
  ImGui::SliderInt("Light Grid Divider##layer", &layers.light_grid_divider, 1,
                   global::kMaxPointLightsPerCluster);

  ImGui::Text("Prepass Linear Depth:");
  ImGui::Toggle("Opaque/AlphaClip##layer", &buttons[kPrepassLinearDepth],
                flags);
  ImGui::Toggle("AlphaBlend##layer", &buttons[kPrepassAlphaBlendLinearDepth],
                flags);

  ImGui::Text("Outline Selected:");
  ImGui::Toggle("View Normals##layer", &buttons[kOutlineNormals], flags);
  ImGui::SameLine();
  ImGui::Toggle("Normalized Depth##layer", &buttons[kOutlineDepth], flags);

  ImGui::Text("Ambient Occlusion:");
  ImGui::Toggle("HBAO##layer", &buttons[kHbao], flags);

  ImGui::Toggle("GTAO Normals##layer", &buttons[kGtaoNormals], flags);
  ImGui::SameLine();
  ImGui::Toggle("GTAO Edges##layer", &buttons[kGtaoEdges], flags);
  ImGui::SameLine();
  ImGui::Toggle("GTAO Visibility##layer", &buttons[kGtaoVisibility], flags);

  ImGui::Text("GBuffer:");
  ImGui::Toggle("Color##layer", &buttons[kColor], flags);
  ImGui::SameLine();
  ImGui::Toggle("Normal##layer", &buttons[kNormal], flags);
  ImGui::SameLine();
  ImGui::Toggle("Roughness##layer", &buttons[kPbrRed], flags);
  ImGui::SameLine();
  ImGui::Toggle("Metalness##layer", &buttons[kPbrGreen], flags);

  ImGui::Text("WBOIT:");
  ImGui::Toggle("Color Accum##layer", &buttons[kTransparencyAccum], flags);
  ImGui::SameLine();
  ImGui::Toggle("Reveal Alpha##layer", &buttons[kTransparencyRevealage], flags);

  ImGui::Text("FXAA:");
  ImGui::Toggle("FXAA##layer", &buttons[kFxaaLuma], flags);

  ImGui::Text("Main HDR:");
  ImGui::Toggle("Bloom##layer", &buttons[kHdr], flags);
  ImGui::SliderInt("MipMap##layer", &layers.bloom_mip_map, 0,
                   global::kBloomDownscaleLimit);

  layers.show_layer = kNone;
  for (int i = 0; i < kTotal; ++i) {
    if (buttons[i]) {
      layers.show_layer = i;
    }
  }
}

void WinSettings::ShowMetrics() {
  const auto& prof = renderer_->profiling_;

  const ImGuiTableFlags flags{ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg};
  if (ImGui::BeginTable("table", 2, flags)) {
    ImGui::TableSetupColumn("Stage");
    ImGui::TableSetupColumn("Time");
    ImGui::TableHeadersRow();
    for (size_t row = 0; row < std::size(prof.time); row++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%s", NamedEnum<Metrics::Enum>::ToStr()[row]);
      ImGui::TableNextColumn();
      ImGui::Text("%.3f ms", prof.time[row]);
    }
    ImGui::EndTable();
  }
  ImGui::Text("Total GPU: %.3f ms", prof.total_time);
}

void WinSettings::ShowDebug() {
  using enum opt::DebugFlags::Enum;
  auto& debug = opt::set::debug;
  ImGui::CheckboxFlags("Lights##debug", &debug.flags, kLights);
  ImGui::SameLine();
  ImGui::CheckboxFlags("Axis##debug", &debug.flags, kAxis);
  ImGui::SameLine();
  ImGui::CheckboxFlags("Normals##debug", &debug.flags, kNormals);

  ImGui::CheckboxFlags("AABB##debug", &debug.flags, kAABB);
  ImGui::SameLine();
  ImGui::CheckboxFlags("OBB##debug", &debug.flags, kOBB);

  ImGui::CheckboxFlags("Cameras##debug", &debug.flags, kCameras);
  ImGui::SameLine();
  ImGui::CheckboxFlags("Particles##debug", &debug.flags, kParticles);

  ImGui::SliderFloat("Point Lights Alpha##debug", &debug.point_lights_alpha,
                     0.01f, 1.0f, "%.2f");
  ImGui::SliderFloat("Normals Magnitude##debug", &debug.normals_magnitude,
                     0.01f, 1.0f, "%.2f");

  ImGui::Separator();

  ImGui::Checkbox("Switch 1##debug", &debug.switch1);
  ImGui::SameLine();
  ImGui::Checkbox("Switch 2##debug", &debug.switch2);
}

void WinSettings::ShowDebugBuffer() {
  if (ImGui::BeginTabBar("Debug Buffer (SSBO)##debug")) {
    if (ImGui::BeginTabItem("int")) {
      for (unsigned int i = 0; i < global::kDebugReadbackSize; i++) {
        ImGui::Text("%u: %d", i, debug_readback_.int_ptr[i]);
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("uint")) {
      for (unsigned int i = 0; i < global::kDebugReadbackSize; i++) {
        ImGui::Text("%u: %u", i, debug_readback_.uint_ptr[i]);
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("float")) {
      for (unsigned int i = 0; i < global::kDebugReadbackSize; i++) {
        ImGui::Text("%u: %.3f", i, debug_readback_.float_ptr[i]);
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("vec4")) {
      for (unsigned int i = 0; i < global::kDebugReadbackSize; i++) {
        const auto& vec4 = debug_readback_.vec4_ptr[i];
        ImGui::Text("%u: [%.3f, %.3f, %.3f, %.3f]", i, vec4.x, vec4.y, vec4.z,
                    vec4.w);
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void WinSettings::ShowMemoryInfo() {
  ImGui::Text("SSBO: %llu MB", mem::MemoryUsed(mem::kBuffer, mem::kMB));
  ImGui::Text("VBO: %llu MB", mem::MemoryUsed(mem::kVertexBuffer, mem::kMB));
  ImGui::Text("TBO: %llu MB", mem::MemoryUsed(mem::kTexture, mem::kMB));
}

void WinSettings::MenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Settings")) {
      if (ImGui::MenuItem("Load from file")) {
        ini::Load(opt::set::GetIniDescription(), "engine.ini");
      }
      if (ImGui::MenuItem("Save settings")) {
        ini::Save(opt::set::GetIniDescription(), "engine.ini");
      }
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
}

void WinSettings::Headers() {
  if (ImGui::CollapsingHeader("OpenGL")) {
    ShowOpenGL();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Pipeline")) {
    ShowPipeline();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Lighting")) {
    ShowLighting();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Shadows")) {
    ShowShadows();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Ambient Occlusion")) {
    ShowAmbientOcclusion();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Postprocess")) {
    ShowPostprocess();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Outline")) {
    ShowOutline();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Crosshair")) {
    ShowCrosshair();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Layers")) {
    ShowLayers();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Metrics")) {
    ShowMetrics();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Debug")) {
    ShowDebug();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("DebugBuffer")) {
    ShowDebugBuffer();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("MemoryInfo")) {
    ShowMemoryInfo();
    ImGui::NewLine();
  }
}

}  // namespace ui