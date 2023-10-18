#pragma once

// global
#include <array>
// local
#include "global.h"
#include "math/collision_types.h"
#include "opengl/uniform_buffer.h"
// fwd
namespace ui {
class WinSettings;
}
class Assets;
class Scene;
class MainCamera;

// layout(std140, binding = x)
struct UniformBinding {
  enum Enum : GLuint {
    kCamera,
    kEngine,
    kLighting,
    kShadows,
    kFrustumCulling,
    kHbao,
    kGtao,
    kTotal
  };
};

/*
https://stackoverflow.com/questions/33589784/ssbo-as-bigger-ubo
Shaders are executed in groups, such that every shader executes in lock-step.
Each group of individual shader invocations has access to a block of memory.
This memory is what UBOs represent.
It is relatively small (on the order of kilobytes),
but accessing it is quite fast.
When performing a rendering operation, data from UBOs are copied into this
shader local memory.

Also, UBOs stored on chip shared memory, access to which is much faster.
The default minimal supported size is 16KB.
GTX 1080 has 64KB.
*/

// std140, align all by 16 bytes
// use vec3 as vec4 or vec3 with 4byte padding
struct UniformCamera {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 proj_inverse;
  glm::mat4 proj_view;
  glm::mat4 frustum_rays_world;
  glm::vec4 position;
  glm::vec4 proj_info;
  GLfloat inv_depth_range;
};

struct UniformEngine {
  // render IBL
  glm::mat4 cubemap_space[global::kCubemapFaces];
  //
  glm::vec2 inv_screen_size;
  GLint enable_hbao;
  GLint enable_gtao;
  // WBOIT
  GLfloat mesh_pow;
  GLfloat mesh_clamp;
  GLfloat particles_pow;
  GLfloat particles_clamp;
  // Outline
  GLfloat depth_threshold;
  GLfloat depth_normal_threshold;
  GLfloat depth_normal_threshold_scale;
  GLfloat normal_threshold;
  glm::vec4 outline_color_width;
  // Postprocess
  GLfloat exposure;
  GLint tonemapping_type;
  GLint enable_blur;
  GLfloat blur_sigma;
  //
  GLint enable_fxaa;
  GLint use_srgb_encoding;
  glm::ivec2 mouse_pos;
  // Debug Normals
  glm::vec4 debug_normals_color_magnitude;
};

struct UniformLighting {
  // frustum clusters (total updaed from SharedAtomicCounters)
  GLuint point_lights_total;
  GLuint point_shadows_total;
  glm::uvec2 cluster_size_px;
  //
  GLfloat slice_scaling_factor;
  GLfloat slice_bias_factor;
  GLfloat empty_pad0;
  GLfloat empty_pad1;
  //
  glm::vec4 cluster_depth_range[global::kFrustumClusters.z];
  // global lighting
  glm::vec4 direction;
  glm::vec4 diffuse;
  glm::vec4 ambient;
  glm::vec4 skybox;
};

struct UniformShadows {
  glm::mat4 light_space[global::kMaxShadowCascades];
  glm::mat4 light_space_bias[global::kMaxShadowCascades];
  glm::vec4 filter_radius_uv;
  glm::vec4 splits_linear;
  //
  GLuint num_cascades;
  GLuint pcf_samples;
  GLfloat point_filter_radius_uv;
  GLfloat empty_pad0;
  //
  glm::vec4 poisson[global::kPoissonMaxSize];
};

struct UniformFrustumCulling {
  glm::uvec4 cmd_offsets[MeshType::kTotal];
  GLuint scene_point_lights_count;
  GLuint scene_mesh_count;
  GLuint scene_instance_count;
  GLuint empty_pad0;
  // pick light
  glm::vec4 ray_origin;
  glm::vec4 ray_dir;
  // vec4 as vec3(plane.normal), plane.dist
  glm::vec4 frustum_planes[global::kFrustumPlanes];
  glm::uvec4 cascades_dop_total;
  glm::vec4
      cascades_dop_planes[global::kMaxShadowCascades * global::kDopPlanes];
};

struct UniformHbao {
  glm::vec2 inv_quarter_resolution;
  GLfloat radius_to_screen;
  GLfloat neg_inv_r2;
  //
  GLfloat bias;
  GLfloat ao_multiplier;
  GLfloat exponent;
  GLfloat blur_sharpness;
  //
  glm::vec4 float2Offsets[global::kRandomElements];
  glm::vec4 jitters[global::kRandomElements];
};

struct UniformGtao {
  GLfloat slice_count;
  GLfloat steps_per_slice;
  glm::vec2 ndc_to_view_mul_pixel_size;
  //
  glm::vec2 ndc_to_view_mul;
  glm::vec2 ndc_to_view_add;
  //
  GLfloat effect_radius;
  GLfloat effect_falloff_range;
  GLfloat radius_multiplier;
  GLfloat final_value_power;
  //
  GLfloat sample_distribution_power;
  GLfloat thin_occluder_compensation;
  GLfloat denoise_blur_beta;
  GLfloat empty_pad0;
};

class UniformBuffers {
 public:
  UniformBuffers(Assets& assets, Scene& scene, MainCamera& camera,
                 ui::WinSettings& ui);

 public:
  gl::UniformBuffer buffer_;
  std::array<DOP, global::kMaxShadowCascades> dop_;

  void InitEngine();
  void InitHbao();
  void UpdateShadowsPoisson();
  void UploadBuffers();

 private:
  Assets& assets_;
  Scene& scene_;
  MainCamera& camera_;
  ui::WinSettings& ui_;

  UniformCamera* ptr_camera_{nullptr};
  UniformEngine* ptr_engine_{nullptr};
  UniformLighting* ptr_lighting_{nullptr};
  UniformShadows* ptr_shadows_{nullptr};
  UniformFrustumCulling* ptr_frustum_culling_{nullptr};
  UniformHbao* ptr_hbao_{nullptr};
  UniformGtao* ptr_gtao_{nullptr};

  void UpdateCamera();
  void UpdateEngine();
  void UpdateLighting();
  void UpdateShadows();
  void UpdateFrustumCulling();
  void UpdateHbao();
  void UpdateGtao();
};
