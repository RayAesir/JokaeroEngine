#include "shader.h"

// deps
#include <spdlog/spdlog.h>
// global
#include <string_view>
// local
#include "global.h"

namespace gl {

void Shader::ClearShaderCache() { shader_cache_.clear(); }

// GLSL Lint use ${workspaceFolder} as current_dir()
// e.g. line #include "shaders/folder/shader.glsl"
fs::path GetPathFromInclude(std::string_view line) {
  // select string between marks
  size_t mark_start = line.find("\"", 0) + 1;
  size_t mark_end = line.find("\"", mark_start);
  std::string_view include{line.substr(mark_start, (mark_end - mark_start))};
  // engine.exe is located inside /engine
  // go to parent_path() from "./" to "../"
  return fs::path("../") / include;
}

struct RangeOfLines {
  size_t first;
  size_t last;
};

enum class ParseTypes {
  kBase,
  kInclude,
};

struct TypedView {
  std::string_view view;
  ParseTypes type;
};

MiVector<RangeOfLines> FindRanges(std::string_view view,
                                  std::string_view to_find,
                                  const RangeOfLines &bounds) {
  MiVector<RangeOfLines> ranges;
  ranges.reserve(8);
  size_t pos = view.find(to_find, bounds.first);
  while (pos != std::string::npos) {
    if (pos > bounds.last) break;
    size_t end_line = view.find("\n", pos);
    if (end_line != std::string::npos) ranges.emplace_back(pos, end_line + 1);
    pos = view.find(to_find, pos + 1);
  }
  return ranges;
}

MiVector<RangeOfLines> InvertRanges(const MiVector<RangeOfLines> &input,
                                    const RangeOfLines &bounds) {
  MiVector<RangeOfLines> ranges;
  if (input.size()) {
    ranges.reserve(input.size() + 1);
    ranges.emplace_back(bounds.first, input[0].first);
    for (size_t i = 1; i < input.size(); ++i) {
      size_t first = input[i - 1].last;
      size_t last = input[i].first;
      if (first != last) ranges.emplace_back(first, last);
    }
    ranges.emplace_back(input.back().last, bounds.last);
  }
  return ranges;
}

MiVector<TypedView> ConvertRangesToViews(std::string_view view,
                                         const MiVector<RangeOfLines> &ranges,
                                         ParseTypes type) {
  MiVector<TypedView> views;
  views.reserve(ranges.size());
  for (const auto [first, last] : ranges) {
    std::string_view sv = view.substr(first, (last - first));
    views.emplace_back(sv, type);
  }
  return views;
}

Shader::SourceCode Shader::ParseShaderFile(const fs::path &full_path) {
  // already loaded - return
  if (const auto it = shader_cache_.find(full_path);
      it != shader_cache_.end()) {
    const auto &cache = it->second;
    return {cache.version.c_str(), cache.body.c_str()};
  }
  // std::string operator+= preallocate memory automatically
  const std::string sources = files::ReadFile(full_path);
  const std::string_view view{sources};
  size_t first_line = 0;
  size_t last_line = view.size();
  // header
  std::string version{""};
  size_t version_line = view.find("#version", 0);
  if (version_line != std::string::npos) {
    // grab #version line
    first_line = view.find("\n", version_line) + 1;
    version += view.substr(version_line, (first_line - version_line));
  }
  // body, process the #include directives
  const std::string_view include{"#include"};
  std::string body{""};
  if (view.find(include, first_line) != std::string::npos) {
    const RangeOfLines bounds{first_line, last_line};
    MiVector<RangeOfLines> inc_ranges = FindRanges(view, include, bounds);
    MiVector<RangeOfLines> base_ranges = InvertRanges(inc_ranges, bounds);
    MiVector<TypedView> inc_views =
        ConvertRangesToViews(view, inc_ranges, ParseTypes::kInclude);
    MiVector<TypedView> base_views =
        ConvertRangesToViews(view, base_ranges, ParseTypes::kBase);
    // compose
    inc_views.reserve(inc_views.size() + base_views.size());
    inc_views.insert(inc_views.end(), base_views.begin(), base_views.end());
    // sort views to restore the original text order
    // the bigger a pointer, the lower a text block in the file
    std::sort(inc_views.begin(), inc_views.end(),
              [](const TypedView &tv1, const TypedView &tv2) {
                return tv1.view.data() < tv2.view.data();
              });
    for (const auto &tv : inc_views) {
      // relative path e.g. "shaders/folder/shader.glsl"
      if (tv.type == ParseTypes::kInclude) {
        fs::path include_path = GetPathFromInclude(tv.view);
        if (const auto it = shader_cache_.find(include_path);
            it != shader_cache_.end()) {
          const auto &cache = it->second;
          body += cache.body;
        } else {
          auto [pair, result] = shader_cache_.try_emplace(
              include_path, std::string(), files::ReadFile(include_path));
          const auto &cache = pair->second;
          body += cache.body;
        }
      }
      if (tv.type == ParseTypes::kBase) {
        body += tv.view;
      }
    }
  } else {
    body += view.substr(first_line, (last_line - first_line));
  }
  // insert new source code into cache
  auto [pair, result] = shader_cache_.try_emplace(full_path, version, body);
  const auto &cache = pair->second;
  return {cache.version.c_str(), cache.body.c_str()};
}

Shader::Shader(const ShaderInfo &params, const std::string &directives) {
  // compile shaders
  SourceCode source_code = Shader::ParseShaderFile(params.path);
  const GLchar *strings[3] = {source_code.version, directives.c_str(),
                              source_code.body};
  shader_ = glCreateShader(params.shader_type);
  glShaderSource(shader_, 3, strings, nullptr);
  glCompileShader(shader_);
  if (!shader_ || !IsValid(params.path.string())) {
    glDeleteShader(shader_);
  }
}

bool Shader::IsValid(const std::string &filepath) {
  GLint status;
  glGetShaderiv(shader_, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) return true;

  GLint info_log_length;
  glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &info_log_length);
  if (info_log_length) {
    MiVector<char> info_log(info_log_length);
    glGetShaderInfoLog(shader_, info_log_length, nullptr, info_log.data());
    spdlog::error("{}: Compilation error '{}'", __FUNCTION__, filepath);
    spdlog::error("{}", info_log.data());
    return GL_FALSE;
  } else {
    spdlog::error("{}: Compilation error '{}'", __FUNCTION__, filepath);
    return GL_FALSE;
  }
}

}  // namespace gl