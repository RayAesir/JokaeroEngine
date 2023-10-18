#include "glsl_header.h"

// deps
#include <fmt/os.h>
#include <spdlog/spdlog.h>
// global
#include <fstream>
#include <regex>
// local
#include "app/parameters.h"
#include "files.h"
#include "global.h"
#include "opengl/buffer_storage.h"
#include "render/uniform_buffers.h"
#include "scene/props.h"
#include "utils/enums.h"

namespace glsl {

// and camelCase
std::string PascalToSnake(std::string_view str) {
  const std::regex pattern("([a-z\\d])([A-Z])");
  constexpr const char *replacement{"$1_$2"};
  return std::regex_replace(str.data(), pattern, replacement);
}

void StreamContent(fmt::v10::ostream &out) {
  constexpr const char *str{"{}"};
  constexpr const char *format{"#define {} {}\n"};
  constexpr const char *format_bind{"#define BIND_{} {}\n"};

  out.print(str, "// UBO binding points\n");
  for (GLuint index = 0; auto sv : NamedEnum<UniformBinding::Enum>::ToStr()) {
    std::string var = PascalToSnake(sv);
    std::transform(var.begin(), var.end(), var.begin(), ::toupper);
    out.print(format_bind, var, index);
    ++index;
  }

  out.print(str, "// SSBO binding points\n");
  for (GLuint index = 0;
       auto sv : NamedEnum<ShaderStorageBinding::Enum>::ToStr()) {
    std::string var = PascalToSnake(sv);
    std::transform(var.begin(), var.end(), var.begin(), ::toupper);
    out.print(format_bind, var, index);
    ++index;
  }

  out.print(str, "// Constants\n");
  out.print(format, "MAX_MESHES", global::kMaxDrawCommands);
  out.print(format, "MAX_INSTANCES", global::kMaxInstances);
  out.print(format, "MAX_POINT_LIGHTS", global::kMaxPointLights);
  out.print(format, "MAX_POINT_SHADOWS", global::kMaxPointShadows);

  out.print(format, "MAX_FX_PRESETS", global::kMaxFxPresets);
  out.print(format, "MAX_PARTICLES_NUM", global::kMaxParticlesNum);
  out.print(format, "MAX_FXS_BUFFER_SIZE", global::kMaxParticlesBufferSize);

  out.print(format, "MAX_CASCADES", global::kMaxShadowCascades);
  out.print(format, "MESH_TYPES", fmt::underlying(MeshType::kTotal));
  out.print(format, "FRUSTUM_PLANES", global::kFrustumPlanes);
  out.print(format, "CUBEMAP_FACES", global::kCubemapFaces);
  out.print(format, "DOP_PLANES", global::kDopPlanes);
  out.print(format, "MAX_POINT_SHADOWS_PER_INSTANCE",
            global::kMaxPointShadowsPerInstance);

  out.print(format, "THREADS_PER_GROUP", global::kThreadsPerGroup);
  out.print(format, "WARP_SIZE", app::gpu.subgroup_size);

  out.print(format, "CLUSTER_THREADS_X", global::kFrustumClustersThreads.x);
  out.print(format, "CLUSTER_THREADS_Y", global::kFrustumClustersThreads.y);
  out.print(format, "CLUSTER_THREADS_Z", global::kFrustumClustersThreads.z);
  out.print(format, "CLUSTERS_PER_X", global::kFrustumClusters.x);
  out.print(format, "CLUSTERS_PER_Y", global::kFrustumClusters.y);
  out.print(format, "CLUSTERS_PER_Z", global::kFrustumClusters.z);
  out.print(format, "CLUSTERS_TOTAL", global::kFrustumClustersCount);
  out.print(format, "MAX_POINT_LIGHTS_PER_CLUSTER",
            global::kMaxPointLightsPerCluster);

  out.print(format, "DEBUG_READBACK_SIZE", global::kDebugReadbackSize);

  out.print(str, "// Props flags\n");
  out.print(format, "ENABLED", fmt::underlying(Props::Flags::kEnabled));
  out.print(format, "VISIBLE", fmt::underlying(Props::Flags::kVisible));
  out.print(format, "CAST_SHADOWS",
            fmt::underlying(Props::Flags::kCastShadows));
}

void Header(const std::string &filename) {
  fs::path path = files::shaders.path / filename;

  // create parent folders if not exist
  const fs::path dir_path = path.parent_path();
  if (!fs::exists(dir_path)) {
    fs::create_directories(dir_path);
    spdlog::info("{}: '{}' create directory for '{}'", __FUNCTION__,
                 dir_path.string(), path.filename().string());
  }

  // create/overwrite file, use bigger fmt buffer with direct format
  // https://www.zverovich.net/2020/08/04/optimal-file-buffer-size.html
  try {
    fmt::v10::ostream out = fmt::output_file(path.string());
    StreamContent(out);
  } catch (std::exception e) {
    spdlog::error("{}: '{}' catch '{}'", __FUNCTION__, path.string(), e.what());
  }
}

}  // namespace glsl