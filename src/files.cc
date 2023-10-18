#include "files.h"

// deps
#include <spdlog/spdlog.h>
// global
#include <fstream>

namespace files {

bool IsValidPath(const fs::path& path) {
  if (!fs::exists(path)) {
    spdlog::error("{}: Does not exist '{}'", __FUNCTION__, path.string());
    return false;
  }
  if (!fs::directory_entry(path).is_regular_file()) {
    spdlog::error("{}: Not a regular file '{}'", __FUNCTION__, path.string());
    return false;
  }
  return true;
}

std::string ReadFile(const fs::path& path) {
  if (!IsValidPath(path)) return "";

  std::ifstream file(path, std::ios_base::binary | std::ios_base::in);
  if (!file.is_open()) {
    spdlog::error("{}: Failed to open '{}'", __FUNCTION__, path.string());
    return "";
  }

  std::string content(std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{});
  if (!file) {
    spdlog::error("{}: Failed to open '{}'", __FUNCTION__, path.string());
    return "";
  }

  return content;
}

size_t GetWriteTimeMS(const fs::path& path) {
  using namespace std::chrono;
  auto time = fs::last_write_time(path);
  auto duration = time.time_since_epoch();
  return duration_cast<milliseconds>(duration).count();
}

size_t DirsCount(const fs::path& path) {
  return std::count_if(fs::recursive_directory_iterator(path),
                       fs::recursive_directory_iterator{},
                       [](const auto& entry) { return entry.is_directory(); });
}

size_t FilesCount(const fs::path& path) {
  return std::count_if(
      fs::recursive_directory_iterator(path),
      fs::recursive_directory_iterator{},
      [](const auto& entry) { return entry.is_regular_file(); });
}

PathInfo::PathInfo(const fs::path& p) : path(p), str(p.string()) {
  if (fs::exists(path)) {
    dirs_count = DirsCount(path);
    files_count = FilesCount(path);
  } else {
    dirs_count = 0;
    files_count = 0;
    spdlog::error("{}: Does not exist '{}'", __FUNCTION__, path.string());
  }
}

MiVector<fs::path> PathInfo::GetFilePaths() const {
  MiVector<fs::path> paths;
  if (fs::exists(path)) {
    paths.reserve(files_count);
    for (const fs::directory_entry& dir_entry :
         fs::recursive_directory_iterator(path)) {
      if (dir_entry.is_regular_file()) {
        paths.emplace_back(dir_entry.path());
      }
    }
  }
  return paths;
}

}  // namespace files