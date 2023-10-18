#pragma once

// global
#include <filesystem>
#include <string>
// local
#include "mi_types.h"

namespace fs = std::filesystem;

namespace files {

bool IsValidPath(const fs::path &path);
std::string ReadFile(const fs::path &path);

size_t GetWriteTimeMS(const fs::path &path);

size_t DirsCount(const fs::path &path);
size_t FilesCount(const fs::path &path);

struct PathInfo {
  PathInfo(const fs::path &p);
  fs::path path;
  std::string str;
  size_t dirs_count;
  size_t files_count;
  MiVector<fs::path> GetFilePaths() const;
};

// all paths are relative to executable
inline const PathInfo shaders{"../shaders"};
inline const PathInfo meshes{"resources/meshes"};
inline const PathInfo textures{"resources/textures"};
inline const PathInfo env_maps{"resources/env_maps"};
inline const PathInfo ibl_archives{"resources/ibl_archives"};
inline const PathInfo particles{"resources/particles"};

}  // namespace files