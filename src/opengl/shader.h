#pragma once

// deps
#include <glad/glad.h>
// global
#include <string>
// local
#include "files.h"
#include "mi_types.h"

namespace gl {

struct ShaderInfo {
  GLenum shader_type;
  fs::path path;
};

// read all files just once and save them in the cache
// temporary object to create Program
class Shader {
 public:
  Shader(const ShaderInfo &params, const std::string &directives);

 public:
  operator GLuint() const { return shader_; }

  // support #define and #include
  struct SourceCode {
    const char *version;
    const char *body;
  };
  static SourceCode ParseShaderFile(const fs::path &full_path);
  static void ClearShaderCache();

 private:
  struct Cache {
    std::string version;
    std::string body;
  };
  inline static MiUnMap<fs::path, Cache> shader_cache_;

  GLuint shader_{0};
  bool IsValid(const std::string &filepath);
};

}  // namespace gl