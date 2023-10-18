#pragma once

// deps
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// local
#include "ui/base_types.h"
// fwd
namespace ini {
struct Description;
}

namespace opt {

struct Layers {
  enum Enum : int {
    kNone,
    kBrdfLut,
    kShowCascade,
    kPointShadow,
    kLightGrid,
    kPrepassLinearDepth,
    kPrepassAlphaBlendLinearDepth,
    kOutlineNormals,
    kOutlineDepth,
    kColor,
    kNormal,
    kPbrRed,
    kPbrGreen,
    kTransparencyAccum,
    kTransparencyRevealage,
    kHbao,
    kGtaoNormals,
    kGtaoEdges,
    kGtaoVisibility,
    kFxaaLuma,
    kHdr,
    kTotal
  };
};

struct DebugFlags {
  enum Enum : int {
    kAxis = (1 << 0),
    kNormals = (1 << 1),
    kAABB = (1 << 2),
    kOBB = (1 << 3),
    kLights = (1 << 4),
    kCameras = (1 << 5),
    kParticles = (1 << 6),
  };
};

struct Tonemapping {
  enum Enum : int {
    kReinhard,
    kReinhardLuma,
    kRomBinDaHouse,
    kFilmic,
    kUncharted2,
    kTotal
  };
};

struct CrosshairFlags {
  enum Enum : int {
    kSquare = (1 << 0),
    kCross = (1 << 1),
    kCircle = (1 << 2),
  };
};

namespace types {

struct Engine {
  Engine();
  float mouse_sensitivity;
};

struct Textures {
  Textures();

  struct Anisotropy {
    float value;
  };
  ui::Selectable<Anisotropy> anisotropy;

  struct LodBias {
    float value;
  };
  ui::Selectable<LodBias> lod_bias;
};

struct Pipeline {
  Pipeline();
  bool toggle_deferred_forward;
  bool toggle_lighting_shadows;
  bool point_lights;
  bool direct_shadowmaps;
  bool point_shadowmaps;
  bool alpha_blend;
  bool particles;
  bool hbao;
  bool gtao;
  bool skybox;
  bool bloom;
};

struct Lighting {
  Lighting();

  // ibl
  bool show_irradiance;
  bool show_prefiltered;
  int environment_map_size;
  int irradiance_map_size;
  int prefilter_max_level;
  int prefilter_map_size;
  int brdf_lut_map_size;
  float cubemap_lod;
  // oit
  float mesh_pow;
  float mesh_clamp;
  float particles_pow;
  float particles_clamp;
};

struct Shadows {
  Shadows();

  struct ShadowsPcf {
    unsigned int tap_count;
  };
  ui::Selectable<ShadowsPcf> pcf_samples;

  struct ShadowmapSize {
    int size;
  };
  ui::Selectable<ShadowmapSize> direct_shadowmap;

  unsigned int num_cascades;
  float split_lambda;
  float sample_offset;
  float polygon_offset_factor;
  float polygon_offset_units;

  ui::Selectable<ShadowmapSize> point_shadowmap;

  float pcf_radius_uv;
  float point_radius_uv;
};

struct ShowLayers {
  ShowLayers();

  int show_layer;
  int shadow_cascade;
  int point_shadow;
  int light_grid_divider;
  int bloom_mip_map;
};

struct Outline {
  Outline();

  float depth_threshold;
  float depth_normal_threshold;
  float depth_normal_threshold_scale;
  float normal_threshold;
  float width;
  glm::vec3 color;
};

struct Postprocess {
  Postprocess();

  bool use_srgb_encoding;
  float exposure;
  struct Tonemapping {
    int type;
  };
  ui::Selectable<Tonemapping> tonemapping;
  // blur
  bool blur;
  float blur_sigma;
  // bloom
  float bloom_threshold;
  float bloom_knee;
  float bloom_intensity;
  // fxaa
  bool fxaa;
  bool fxaa_quality;
};

struct Hbao {
  Hbao();

  float radius;
  float intensity;
  float bias;
  float blur_sharpness;
  glm::ivec2 quater_size;
};

struct Gtao {
  Gtao();

  struct GtaoPreset {
    float slice_count;
    float steps_per_slice;
  };
  ui::Selectable<GtaoPreset> presets;

  float slice_count;
  float steps_per_slice;
  float effect_radius;
  float effect_falloff_range;
  float radius_multiplier;
  float final_value_power;
  float sample_distribution_power;
  float thin_occluder_compensation;
  float denoise_blur_beta;
  bool two_pass_blur;
};

struct Crosshair {
  Crosshair();

  int flags;
  float size;
  float inner_size;
  float thickness;
  glm::vec4 color;
};

struct Debug {
  Debug();

  glm::vec3 normals_color;
  float normals_magnitude;
  float point_lights_alpha;
  int flags;
  bool switch1;
  bool switch2;
};

}  // namespace types

// rw protected access
namespace hidden {

inline types::Engine gEngine;
inline types::Textures gTextures;
inline types::Pipeline gPipeline;
inline types::Lighting gLighting;
inline types::Shadows gShadows;
inline types::ShowLayers gLayers;
inline types::Outline gOutline;
inline types::Postprocess gPostprocess;
inline types::Hbao gHbao;
inline types::Gtao gGtao;
inline types::Crosshair gCrosshair;
inline types::Debug gDebug;

}  // namespace hidden

// emphasize that global variables is readonly
inline const types::Engine& engine = hidden::gEngine;
inline const types::Textures& textures = hidden::gTextures;
inline const types::Pipeline& pipeline = hidden::gPipeline;
inline const types::Lighting& lighting = hidden::gLighting;
inline const types::Shadows& shadows = hidden::gShadows;
inline const types::ShowLayers& layers = hidden::gLayers;
inline const types::Outline& outline = hidden::gOutline;
inline const types::Postprocess& postprocess = hidden::gPostprocess;
inline const types::Hbao& hbao = hidden::gHbao;
inline const types::Gtao& gtao = hidden::gGtao;
inline const types::Crosshair& crosshair = hidden::gCrosshair;
inline const types::Debug& debug = hidden::gDebug;

namespace set {

inline types::Engine& engine = hidden::gEngine;
inline types::Textures& textures = hidden::gTextures;
inline types::Pipeline& pipeline = hidden::gPipeline;
inline types::Lighting& lighting = hidden::gLighting;
inline types::Shadows& shadows = hidden::gShadows;
inline types::ShowLayers& layers = hidden::gLayers;
inline types::Outline& outline = hidden::gOutline;
inline types::Postprocess& postprocess = hidden::gPostprocess;
inline types::Hbao& hbao = hidden::gHbao;
inline types::Gtao& gtao = hidden::gGtao;
inline types::Crosshair& crosshair = hidden::gCrosshair;
inline types::Debug& debug = hidden::gDebug;

void UpdateScreenSize();
ini::Description GetIniDescription();

}  // namespace set

}  // namespace opt