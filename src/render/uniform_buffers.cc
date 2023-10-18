#include "uniform_buffers.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "app/input.h"
#include "app/parameters.h"
#include "assets/assets.h"
#include "math/random.h"
#include "options.h"
#include "render/poisson.h"
#include "scene/scene.h"
#include "ui/win_settings.h"

UniformBuffers::UniformBuffers(Assets &assets, Scene &scene, MainCamera &camera,
                               ui::WinSettings &ui)
    : assets_(assets), scene_(scene), camera_(camera), ui_(ui) {
  using enum UniformBinding::Enum;

  ptr_camera_ = buffer_.MapBuffer<UniformCamera>(kCamera);
  ptr_engine_ = buffer_.MapBuffer<UniformEngine>(kEngine);
  ptr_lighting_ = buffer_.MapBuffer<UniformLighting>(kLighting);
  ptr_shadows_ = buffer_.MapBuffer<UniformShadows>(kShadows);
  ptr_frustum_culling_ =
      buffer_.MapBuffer<UniformFrustumCulling>(kFrustumCulling);
  ptr_hbao_ = buffer_.MapBuffer<UniformHbao>(kHbao);
  ptr_gtao_ = buffer_.MapBuffer<UniformGtao>(kGtao);
}

void UniformBuffers::UploadBuffers() {
  UpdateCamera();
  UpdateEngine();
  UpdateLighting();
  UpdateShadows();
  UpdateFrustumCulling();
  UpdateHbao();
  UpdateGtao();
  buffer_.Upload();
}

void UniformBuffers::UpdateCamera() {
  auto &ubo = *ptr_camera_;

  ubo.view = camera_.view_;
  ubo.proj = camera_.proj_;
  ubo.proj_inverse = camera_.proj_inverse_;
  ubo.proj_view = camera_.proj_view_;
  ubo.position = glm::vec4(camera_.GetPosition(), 1.0f);
  ubo.frustum_rays_world = camera_.GetFrustumRaysWS();
  // reconstruct the view space from linear depth
  // window space to NDC: Zd = 2.0 * depth - 1.0
  // NDC to view space: Zvs = proj[3][2] / (proj[2][2] + Zd)
  // X and Y to view space:
  // Zvs * Xd / proj[0][0]
  // Zvs * Yd / proj[1][1]
  // proj[0][0] = 1.0 / tan(half_fov) * aspect
  // proj[1][1] = 1.0 / tan(half_fov)
  // this is optimized variant to avoid NDC conversion
  ubo.proj_info.x = 2.0f / camera_.proj_[0][0];
  ubo.proj_info.y = 2.0f / camera_.proj_[1][1];
  ubo.proj_info.z = -1.0f / camera_.proj_[0][0];
  ubo.proj_info.w = -1.0f / camera_.proj_[1][1];
  ubo.inv_depth_range = 1.0f / camera_.DepthRange();
}

void UniformBuffers::InitEngine() {
  auto &ubo = *ptr_engine_;

  // IBL, point shadowmaps and cubemap conversion
  static constexpr glm::vec3 kLookAtCenter[global::kCubemapFaces]{
      glm::vec3(1.0f, 0.0f, 0.0f),   // +X
      glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
      glm::vec3(0.0f, 1.0f, 0.0f),   // +Y
      glm::vec3(0.0f, -1.0f, 0.0f),  // -Y
      glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
      glm::vec3(0.0f, 0.0f, -1.0f),  // -Z
  };

  // orientation
  static constexpr glm::vec3 kLookAtUp[global::kCubemapFaces]{
      glm::vec3(0.0f, -1.0f, 0.0f),  // +X: [-Y, -Z]
      glm::vec3(0.0f, -1.0f, 0.0f),  // -X: [-Y, +Z]
      glm::vec3(0.0f, 0.0f, 1.0f),   // +Y: [+X, +Z]
      glm::vec3(0.0f, 0.0f, -1.0f),  // -Y: [+X, -Z]
      glm::vec3(0.0f, -1.0f, 0.0f),  // +Z: [+X, -Y]
      glm::vec3(0.0f, -1.0f, 0.0f),  // -Z: [-X, -Y]
  };

  glm::mat4 cubemap_proj =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  for (int i = 0; i < global::kCubemapFaces; ++i) {
    glm::mat4 cubemap_view =
        glm::lookAt(glm::vec3(0.0f), kLookAtCenter[i], kLookAtUp[i]);
    ubo.cubemap_space[i] = cubemap_proj * cubemap_view;
  }
}

void UniformBuffers::UpdateEngine() {
  auto &ubo = *ptr_engine_;

  ubo.inv_screen_size = app::opengl.inv_framebuffer_size;
  ubo.enable_hbao = opt::pipeline.hbao;
  ubo.enable_gtao = opt::pipeline.gtao;

  const auto &lighting = opt::lighting;
  ubo.mesh_pow = lighting.mesh_pow;
  ubo.mesh_clamp = lighting.mesh_clamp;
  ubo.particles_pow = lighting.particles_pow;
  ubo.particles_clamp = lighting.particles_clamp;

  const auto &outline = opt::outline;
  ubo.depth_threshold = outline.depth_threshold;
  ubo.depth_normal_threshold = outline.depth_normal_threshold;
  ubo.depth_normal_threshold_scale = outline.depth_normal_threshold_scale;
  ubo.normal_threshold = outline.normal_threshold;
  ubo.outline_color_width = glm::vec4(outline.color, outline.width);

  const auto &postprocess = opt::postprocess;
  ubo.exposure = postprocess.exposure;
  ubo.tonemapping_type = postprocess.tonemapping.Selected().type;
  ubo.enable_blur = postprocess.blur;
  ubo.blur_sigma = postprocess.blur_sigma;
  ubo.enable_fxaa = postprocess.fxaa;
  ubo.use_srgb_encoding = postprocess.use_srgb_encoding;

  const auto &debug = opt::debug;
  ubo.debug_normals_color_magnitude =
      glm::vec4(debug.normals_color, debug.normals_magnitude);

  // calculate correct mouse position, 2D and 3D, for selection
  if (app::imgui.ui_active) {
    // e.g. framebuffer size is 1920x1080 and window size is 1280x720
    glm::vec2 scale = glm::vec2(app::opengl.framebuffer_size) /
                      glm::vec2(app::glfw.window_size);
    ubo.mouse_pos = glm::ivec2(input::GetMousePos() * scale);
  } else {
    ubo.mouse_pos = app::opengl.framebuffer_size / 2;
  }
};

void UniformBuffers::UpdateLighting() {
  const auto &camera = camera_;
  auto &ubo = *ptr_lighting_;

  // direct lighting is global, negate vector
  // light_pos == world_center (0.0, 0.0, 0.0)
  const auto &light = scene_.lighting_;
  glm::vec3 light_dir = -light.direction;
  ubo.direction = glm::vec4(light_dir, 1.0f);
  ubo.diffuse = light.diffuse;
  ubo.ambient = light.ambient;
  ubo.skybox = light.skybox;

  // frustum clusters
  ubo.cluster_size_px = glm::uvec2{
      CeilInt(app::opengl.framebuffer_size.x, global::kFrustumClusters.x),  //
      CeilInt(app::opengl.framebuffer_size.y, global::kFrustumClusters.y),  //
  };

  float ratio = camera.DepthRatio();
  constexpr float slices = static_cast<float>(global::kFrustumClusters.z);
  ubo.slice_scaling_factor = slices / glm::log(ratio);
  ubo.slice_bias_factor =
      -(slices * glm::log(camera.GetNear()) / glm::log(ratio));
  // view space z is negative
  for (unsigned int z = 0; z < global::kFrustumClusters.z; ++z) {
    ubo.cluster_depth_range[z][0] =
        -camera.GetNear() * glm::pow(ratio, static_cast<float>(z) / slices);
    ubo.cluster_depth_range[z][1] =
        -camera.GetNear() * glm::pow(ratio, static_cast<float>(z + 1) / slices);
  }
};

void UniformBuffers::UpdateShadows() {
  auto &ubo = *ptr_shadows_;
  const auto &light_dir = scene_.lighting_.direction;

  const auto &shadows = opt::shadows;
  const float near = camera_.GetNear();
  const float clip_range = camera_.DepthRange();
  const float ratio = camera_.DepthRatio();
  static constexpr glm::vec3 kWorldUp{0.0f, 1.0f, 0.0f};
  const float map_half_size =
      static_cast<float>(shadows.direct_shadowmap.Selected().size) * 0.5f;

  // homogeneous to screen coordinates optimization
  // using glClipControl, now Zd is in [0, 1] range
  static constexpr glm::mat4 kBias = {
      0.5f, 0.0f, 0.0f, 0.0f,  //
      0.0f, 0.5f, 0.0f, 0.0f,  //
      0.0f, 0.0f, 1.0f, 0.0f,  //
      0.5f, 0.5f, 0.0f, 1.0f   //
  };

  float last_split_dist = near;
  for (unsigned int i = 0; i < shadows.num_cascades; ++i) {
    // update the splits
    float si = (i + 1) / static_cast<float>(shadows.num_cascades);
    float log = near * glm::pow(ratio, si);
    float uni = near + clip_range * si;
    float split_dist = shadows.split_lambda * (log - uni) + uni;

    // build the tight DOP for shadow culling
    Corners corners = camera_.GetFrustumCornersWS(last_split_dist, split_dist);
    glm::mat4 cascade_proj =
        glm::perspective(camera_.GetFov(), app::opengl.aspect_ratio,
                         last_split_dist, split_dist);
    Frustum frustum{cascade_proj * camera_.view_};
    dop_[i] = DOP{frustum, corners, light_dir};

    // to render stable shadow maps we need the rotation-invariant frustum
    // where width and height don't change every frame
    // the calculation of bounding sphere around the frustum gives us more
    // precision/quality (~10%) over the frustum corners method
    // https://gamedev.stackexchange.com/questions/73851/how-do-i-fit-the-camera-frustum-inside-directional-light-space
    // https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
    Sphere sphere = camera_.GetBoundingSphere(last_split_dist, split_dist);
    glm::vec3 &center = sphere.center_;
    float radius = sphere.radius_;
    float frustum_size = radius * 2.0f;

    // the difference between the rounded and actual tex coordinate is the
    // amount by which we need to translate the shadow matrix in order to
    // cancel sub-texel movement (shadow swimming)
    glm::mat4 light_view =
        glm::lookAt(center - (light_dir * radius), center, kWorldUp);
    glm::vec4 offset = glm::mod(light_view[3], radius / map_half_size);
    offset.w = 0.0f;
    light_view[3] -= offset;
    glm::mat4 light_proj =
        glm::ortho(-radius, radius, -radius, radius, 0.0f, frustum_size);

    glm::mat4 light_space = light_proj * light_view;

    // ubo
    ubo.light_space[i] = light_space;
    ubo.light_space_bias[i] = kBias * light_space;
    ubo.filter_radius_uv[i] = shadows.pcf_radius_uv / radius;
    ubo.splits_linear[i] = split_dist;

    // ui
    auto &csm = ui_.csm_;
    csm.center[i] = center;
    csm.radius[i] = radius;
    csm.split[i] = split_dist;
    csm.dop_total[i] = dop_[i].total_;
    for (unsigned int p = 0; p < dop_[i].total_; ++p) {
      const auto &plane = dop_[i].planes_[p];
      csm.dop_planes[i][p] = glm::vec4(plane.normal_, plane.dist_);
    }

    // pass the distance
    last_split_dist = split_dist;
  }

  ubo.num_cascades = shadows.num_cascades;
  ubo.pcf_samples = shadows.pcf_samples.Selected().tap_count;
  ubo.point_filter_radius_uv = shadows.point_radius_uv;
}

void UniformBuffers::UpdateFrustumCulling() {
  auto &ubo = *ptr_frustum_culling_;

  for (unsigned int i = 0; i < MeshType::kTotal; ++i) {
    ubo.cmd_offsets[i].x = assets_.models_.GetMeshOffsets()[i];
  }
  ubo.scene_point_lights_count = scene_.point_light_count_;
  ubo.scene_mesh_count = assets_.models_.GetMeshTotal();
  ubo.scene_instance_count = scene_.instance_count_;

  ubo.ray_origin = glm::vec4(camera_.ray_.origin_, 0.0f);
  ubo.ray_dir = glm::vec4(camera_.ray_.dir_, 0.0f);

  // frustum culling
  for (unsigned int i = 0; i < global::kFrustumPlanes; ++i) {
    const auto &planes = camera_.frustum_.planes_;
    ubo.frustum_planes[i] = glm::vec4(planes[i].normal_, planes[i].dist_);
  }

  for (unsigned int c = 0; c < opt::shadows.num_cascades; ++c) {
    ubo.cascades_dop_total[c] = static_cast<GLuint>(dop_[c].total_);

    const auto &planes = dop_[c].planes_;
    for (unsigned int p = 0; p < dop_[c].total_; ++p) {
      unsigned int flat_index = (c * global::kDopPlanes) + p;
      ubo.cascades_dop_planes[flat_index] =
          glm::vec4(planes[p].normal_, planes[p].dist_);
    }
  }
}

void UniformBuffers::InitHbao() {
  auto &ubo = *ptr_hbao_;

  // init static arrays
  MiVector<glm::vec4> random_uniform;
  rnd::GenerateRandomHbao(global::kRandomElements, random_uniform);
  for (int i = 0; i < global::kRandomElements; ++i) {
    ubo.float2Offsets[i].x = static_cast<float>(i % 4) + 0.5f;
    ubo.float2Offsets[i].y = static_cast<float>(i / 4) + 0.5f;
    ubo.jitters[i] = random_uniform[i];
  }
}

void UniformBuffers::UpdateHbao() {
  auto &ubo = *ptr_hbao_;

  float proj_scale = static_cast<float>(app::opengl.framebuffer_size.y) /
                     (glm::tan(camera_.GetFov() * 0.5f) * 2.0f);
  const auto &options = opt::hbao;
  ubo.inv_quarter_resolution = 1.0f / glm::vec2(options.quater_size);
  // radius
  ubo.radius_to_screen = 0.5f * options.radius * proj_scale;
  ubo.neg_inv_r2 = -1.0f / (options.radius * options.radius);
  // ambient occlusion
  ubo.exponent = options.intensity;
  ubo.bias = options.bias;
  ubo.ao_multiplier = 1.0f / (1.0f - options.bias);
  ubo.blur_sharpness = options.blur_sharpness;
}

void UniformBuffers::UpdateGtao() {
  auto &ubo = *ptr_gtao_;

  const auto &gtao = opt::gtao;
  ubo.slice_count = gtao.slice_count;
  ubo.steps_per_slice = gtao.steps_per_slice;

  // OpenGL adaptation of https://github.com/GameTechDev/XeGTAO
  // this part in DirectX coodinate system (left-handed)
  float tan_half_fov_x = 1.0f / camera_.proj_[0][0];
  float tan_half_fov_y = 1.0f / camera_.proj_[1][1];
  ubo.ndc_to_view_mul =
      glm::vec2(2.0f * tan_half_fov_x, -2.0f * tan_half_fov_y);
  ubo.ndc_to_view_add =
      glm::vec2(-1.0f * tan_half_fov_x, 1.0f * tan_half_fov_y);
  ubo.ndc_to_view_mul_pixel_size =
      ubo.ndc_to_view_mul * app::opengl.inv_framebuffer_size;

  ubo.effect_radius = gtao.effect_radius;
  ubo.effect_falloff_range = gtao.effect_falloff_range;
  ubo.radius_multiplier = gtao.radius_multiplier;
  ubo.final_value_power = gtao.final_value_power;

  ubo.sample_distribution_power = gtao.sample_distribution_power;
  ubo.thin_occluder_compensation = gtao.thin_occluder_compensation;
  ubo.denoise_blur_beta = gtao.denoise_blur_beta;
}

void UniformBuffers::UpdateShadowsPoisson() {
  using namespace poisson;
  auto &ubo = *ptr_shadows_;

  switch (opt::shadows.pcf_samples.current) {
    case 0:
      std::copy(std::begin(kPoisson25), std::end(kPoisson25),
                std::begin(ubo.poisson));
      break;
    case 1:
      std::copy(std::begin(kPoisson32), std::end(kPoisson32),
                std::begin(ubo.poisson));
      break;
    case 2:
      std::copy(std::begin(kPoisson64), std::end(kPoisson64),
                std::begin(ubo.poisson));
      break;
    case 3:
      std::copy(std::begin(kPoisson100), std::end(kPoisson100),
                std::begin(ubo.poisson));
      break;
    case 4:
      std::copy(std::begin(kPoisson128), std::end(kPoisson128),
                std::begin(ubo.poisson));
      break;
    default:
      break;
  }
}