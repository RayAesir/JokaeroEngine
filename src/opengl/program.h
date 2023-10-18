#pragma once

// deps
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// local
#include "mi_types.h"
#include "shader.h"

namespace gl {

/*
Glslang, Khronos' reference GLSL compiler/validator,
uses the following extensions to determine what type of shader
that the file is for:
.vert - a vertex shader
.tesc - a tessellation control shader
.tese - a tessellation evaluation shader
.geom - a geometry shader
.frag - a fragment shader
.comp - a compute shader
*/

class Program {
 public:
  // input can be filepaths or defines in any order
  Program(std::initializer_list<std::string_view> list);
  Program(const Program &program, std::string_view directives);
  ~Program();
  // OpenGL resources (move-only)
  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;
  Program(Program &&o) noexcept;
  Program &operator=(Program &&o) noexcept;

 public:
  static void InitAll();
  // check write_time and reload only modified programs
  static void ReloadAll();

  operator GLuint() const { return program_; }
  void Use() const;
  void SetPreprocessorDirectives(const std::string &directives);
  void Load();
  void Reload();
  std::string GetName() const noexcept;
  void UpdateWriteTime();
  size_t GetWriteTime() const;

  void SetUniform(const std::string &name, GLboolean value) const;
  void SetUniform(const std::string &name, GLfloat value) const;
  void SetUniform(const std::string &name, GLint value) const;
  void SetUniform(const std::string &name, GLuint value) const;

  void SetUniform(const std::string &name, const glm::vec2 &value) const;
  void SetUniform(const std::string &name, const glm::ivec2 &value) const;
  void SetUniform(const std::string &name, const glm::uvec2 &value) const;

  void SetUniform(const std::string &name, const glm::vec3 &value) const;
  void SetUniform(const std::string &name, const glm::ivec3 &value) const;
  void SetUniform(const std::string &name, const glm::uvec3 &value) const;

  void SetUniform(const std::string &name, const glm::vec4 &value) const;
  void SetUniform(const std::string &name, const glm::ivec4 &value) const;
  void SetUniform(const std::string &name, const glm::uvec4 &value) const;

  void SetUniform(const std::string &name, const glm::mat3 &value) const;
  void SetUniform(const std::string &name, const glm::mat4 &value) const;

 protected:
  inline static MiVector<Program *> programs_;

  GLuint program_{0};
  MiVector<ShaderInfo> params_;
  std::string directives_;
  MiUnMap<std::string, GLint> uniform_locations_;
  size_t write_time_ms_{0};

  bool IsValid();
  void RetrieveUniforms() noexcept;
};

struct SubroutineInterface {
  GLenum subroutine;
  GLenum shader_type;
};

struct SubroutinesPerStage {
  // the current subroutines parameters to send
  // the vector index as the location index
  MiVector<GLuint> to_gpu;
  // to find names
  MiUnMap<std::string, GLuint> locations_indices;
  MiUnMap<std::string, GLuint> subroutines_indices;
};

class ProgramSubroutines : public Program {
 public:
  ProgramSubroutines(std::initializer_list<std::string_view> list);
  // OpenGL resources (move-only)
  ProgramSubroutines(const ProgramSubroutines &) = delete;
  ProgramSubroutines &operator=(const ProgramSubroutines &) = delete;
  ProgramSubroutines(ProgramSubroutines &&o) noexcept;
  ProgramSubroutines &operator=(ProgramSubroutines &&) = delete;

 public:
  operator GLuint() const { return program_; }
  void Reload();
  // example:
  // 4 subroutines represented as the array of indices: [0, 1, 2, 3]
  // 2 active uniform locations: (location = 0), (location = 1)
  // send to GPU:
  // num of locations == 2 and array of subroutines' indices = [3, 2]
  void SetSubroutineToLocation(GLenum shader_type, const std::string &sub_name,
                               const std::string &loc_name);
  void UseSubroutines(GLenum shader_type);

 private:
  MiVector<SubroutineInterface> interfaces_;
  MiUnMap<GLenum, SubroutinesPerStage> subroutines_;
  void SubroutineInterface() noexcept;
  void RetrieveSubroutines() noexcept;
};

}  // namespace gl