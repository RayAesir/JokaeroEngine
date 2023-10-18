#include "program.h"

// deps
#include <spdlog/spdlog.h>

#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
// local
#include "files.h"

namespace gl {

void Program::InitAll() {
  using namespace indicators;
  indicators::show_console_cursor(false);

  const size_t total = programs_.size();
  indicators::ProgressBar bar{
      option::BarWidth{80},
      option::Start{"["},
      option::Lead{"#"},
      option::Remainder{" "},
      option::End{"]"},
      option::ForegroundColor{Color::white},
      option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
      option::MaxProgress{total}};

  std::string label;
  for (size_t i = 0; i < total; ++i) {
    programs_[i]->Load();
    label = fmt::format("{}/{} Loading Shaders", i + 1, total);
    bar.set_option(option::PostfixText{label});
    bar.tick();
  }
  Shader::ClearShaderCache();

  indicators::show_console_cursor(true);
}

void Program::ReloadAll() {
  for (Program *program : programs_) {
    size_t last_time = program->GetWriteTime();
    program->UpdateWriteTime();
    size_t new_time = program->GetWriteTime();
    if (last_time != new_time) {
      program->Reload();
      spdlog::info("Reload::[{}]", program->GetName());
    }
  }
  Shader::ClearShaderCache();
}

GLenum ShaderTypeFromPath(std::string_view path) {
  static constexpr const char *ext[6]{
      ".comp",  //
      ".vert",  //
      ".frag",  //
      ".geom",  //
      ".tesc",  //
      ".tese"   //
  };
  static constexpr GLenum types[6]{
      GL_COMPUTE_SHADER,         //
      GL_VERTEX_SHADER,          //
      GL_FRAGMENT_SHADER,        //
      GL_GEOMETRY_SHADER,        //
      GL_TESS_CONTROL_SHADER,    //
      GL_TESS_EVALUATION_SHADER  //
  };
  for (int i = 0; i < 6; ++i) {
    if (path.ends_with(ext[i])) {
      return types[i];
    }
  }
  return 0;
}

Program::Program(std::initializer_list<std::string_view> list) {
  params_.reserve(list.size());
  for (const auto &sv : list) {
    // process shaders
    GLenum shader_type = ShaderTypeFromPath(sv);
    if (shader_type) {
      fs::path full_path = files::shaders.path / sv;
      if (files::IsValidPath(full_path)) {
        params_.emplace_back(shader_type, full_path);
      }
    }
    // process directives
    if (sv.starts_with('#')) {
      directives_ += sv;
    }
  }
  // after parsing arguments
  UpdateWriteTime();
  programs_.push_back(this);
}

Program::Program(const Program &program, std::string_view directives)
    : params_(program.params_),
      write_time_ms_(program.write_time_ms_),
      directives_(directives) {
  programs_.push_back(this);
}

Program::~Program() {
  std::erase(programs_, this);
  glDeleteProgram(program_);
}

Program::Program(Program &&o) noexcept
    : program_(std::exchange(o.program_, 0)),
      params_(std::move(o.params_)),
      directives_(std::move(o.directives_)),
      uniform_locations_(std::move(o.uniform_locations_)),
      write_time_ms_(std::exchange(o.write_time_ms_, 0)) {
  std::erase(programs_, &o);
  programs_.push_back(this);
}

Program &Program::operator=(Program &&o) noexcept {
  if (&o == this) return *this;
  if (program_) glDeleteProgram(program_);

  program_ = std::exchange(o.program_, 0);
  params_ = std::move(o.params_);
  directives_ = std::move(o.directives_);
  uniform_locations_ = std::move(o.uniform_locations_);
  write_time_ms_ = std::exchange(o.write_time_ms_, 0);

  std::erase(programs_, &o);
  programs_.push_back(this);

  return *this;
}

void Program::Use() const { glUseProgram(program_); }

void Program::SetPreprocessorDirectives(const std::string &directives) {
  directives_ = directives;
}

void Program::Load() {
  // delete old, load new
  glDeleteProgram(program_);
  MiVector<GLuint> shaders(params_.size());
  program_ = glCreateProgram();

  // create shaders
  for (size_t i = 0; i < params_.size(); ++i) {
    shaders[i] = Shader(params_[i], directives_);
    glAttachShader(program_, shaders[i]);
  }

  // link
  glLinkProgram(program_);

  // check
  if (program_ || IsValid()) {
    RetrieveUniforms();
  } else {
    glDeleteProgram(program_);
  }

  // clean up
  for (GLuint shader : shaders) {
    glDetachShader(program_, shader);
    glDeleteShader(shader);
  }
}

void Program::Reload() {
  glDeleteProgram(program_);
  Load();
}

std::string Program::GetName() const noexcept {
  MiVector<std::string> name_parts;
  name_parts.reserve(params_.size());
  for (const auto &shader : params_) {
    name_parts.emplace_back(shader.path.filename().string());
  }

  if (directives_.size()) {
    std::string dir = directives_;
    // remove '\n', '\r', etc.
    std::erase_if(dir, [](char c) {
      return std::iscntrl(static_cast<unsigned char>(c));
    });
    return fmt::format("{} : {}", fmt::join(name_parts, "+"), dir);
  } else {
    return fmt::format("{}", fmt::join(name_parts, "+"));
  }
}

void Program::UpdateWriteTime() {
  write_time_ms_ = 0;
  // all paths already valid
  for (const auto &shader : params_) {
    // sum overflow doesn't matter, only equality
    write_time_ms_ += files::GetWriteTimeMS(shader.path);
  }
}

size_t Program::GetWriteTime() const { return write_time_ms_; }

bool Program::IsValid() {
  GLint status;
  glGetProgramiv(program_, GL_LINK_STATUS, &status);
  if (status == GL_TRUE) return true;

  GLint info_log_length;
  glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &info_log_length);
  if (info_log_length) {
    MiVector<char> info_log(info_log_length);
    glGetProgramInfoLog(program_, info_log_length, nullptr, info_log.data());
    spdlog::error("{}: Program linking", __FUNCTION__);
    spdlog::error("{}", info_log.data());
    return GL_FALSE;
  } else {
    spdlog::error("{}: Program linking", __FUNCTION__);
    return GL_FALSE;
  }
}

void Program::RetrieveUniforms() noexcept {
  uniform_locations_.clear();

  GLint num_uniforms = 0;
  glGetProgramInterfaceiv(program_, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &num_uniforms);
  if (num_uniforms > 0) {
    const GLenum properties[] = {GL_NAME_LENGTH, GL_LOCATION};
    const GLint properties_size = static_cast<GLint>(std::size(properties));
    MiVector<char> name_data(64);
    for (GLint i = 0; i < num_uniforms; ++i) {
      GLint values[properties_size];
      glGetProgramResourceiv(program_, GL_UNIFORM, i, properties_size,
                             properties, properties_size, nullptr, values);
      // uniforms in uniform blocks have the value = -1
      // other input and output variables have the value = -1
      // the texture samplers are counted as uniforms
      if (values[1] >= 0) {
        name_data.resize(values[0]);
        glGetProgramResourceName(program_, GL_UNIFORM, i,
                                 static_cast<GLsizei>(name_data.size()),
                                 nullptr, name_data.data());
        std::string uniform_name(name_data.begin(), name_data.end() - 1);

        GLint uniform_location = values[1];
        uniform_locations_.try_emplace(uniform_name, uniform_location);
      }
    }
  }
}

void Program::SetUniform(const std::string &name, GLboolean value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform1i(program_, it->second, static_cast<GLint>(value));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name, GLfloat value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform1f(program_, it->second, value);
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name, GLint value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform1i(program_, it->second, value);
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name, GLuint value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform1ui(program_, it->second, value);
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::vec2 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform2fv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::ivec2 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform2iv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::uvec2 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform2uiv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::vec3 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform3fv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::ivec3 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform3iv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::uvec3 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform3uiv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::vec4 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform4fv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::ivec4 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform4iv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::uvec4 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniform4uiv(program_, it->second, 1, &(value[0]));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::mat3 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniformMatrix3fv(program_, it->second, 1, GL_FALSE, &(value[0].x));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

void Program::SetUniform(const std::string &name,
                         const glm::mat4 &value) const {
  auto it = uniform_locations_.find(name);
  if (it != uniform_locations_.end()) {
    glProgramUniformMatrix4fv(program_, it->second, 1, GL_FALSE, &(value[0].x));
  } else {
    spdlog::warn("{}: Uniform not found '{}'", __FUNCTION__, name);
  }
}

ProgramSubroutines::ProgramSubroutines(
    std::initializer_list<std::string_view> list)
    : Program(list) {
  SubroutineInterface();
  RetrieveSubroutines();
}

ProgramSubroutines::ProgramSubroutines(ProgramSubroutines &&o) noexcept
    : Program(std::move(o)),
      interfaces_(std::move(o.interfaces_)),
      subroutines_(std::move(o.subroutines_)) {}

void ProgramSubroutines::Reload() {
  glDeleteProgram(program_);
  Load();
  RetrieveSubroutines();
}

void ProgramSubroutines::SubroutineInterface() noexcept {
  interfaces_.reserve(params_.size());
  for (const auto &param : params_) {
    switch (param.shader_type) {
      case GL_VERTEX_SHADER:
        interfaces_.emplace_back(GL_VERTEX_SUBROUTINE, param.shader_type);
        break;
      case GL_TESS_CONTROL_SHADER:
        interfaces_.emplace_back(GL_TESS_CONTROL_SUBROUTINE, param.shader_type);
        break;
      case GL_TESS_EVALUATION_SHADER:
        interfaces_.emplace_back(GL_TESS_EVALUATION_SUBROUTINE,
                                 param.shader_type);
        break;
      case GL_GEOMETRY_SHADER:
        interfaces_.emplace_back(GL_GEOMETRY_SUBROUTINE, param.shader_type);
        break;
      case GL_FRAGMENT_SHADER:
        interfaces_.emplace_back(GL_FRAGMENT_SUBROUTINE, param.shader_type);
        break;
      default:
        break;
    }
  }
}

void ProgramSubroutines::RetrieveSubroutines() noexcept {
  for (auto &[stage, info] : subroutines_) {
    info.locations_indices.clear();
    info.subroutines_indices.clear();
    info.to_gpu.clear();
  }

  for (const auto &iface : interfaces_) {
    GLint loc_count;
    glGetProgramStageiv(program_, iface.shader_type,
                        GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &loc_count);
    GLint sub_count;
    glGetProgramStageiv(program_, iface.shader_type, GL_ACTIVE_SUBROUTINES,
                        &sub_count);

    if (0 < sub_count + loc_count) {
      auto [inserted, success] = subroutines_.try_emplace(iface.shader_type);
      auto &sub_indices = inserted->second.subroutines_indices;
      auto &loc_indices = inserted->second.locations_indices;
      auto &to_gpu = inserted->second.to_gpu;
      to_gpu.resize(loc_count);

      GLint sub_name_max_len;
      glGetProgramStageiv(program_, iface.shader_type,
                          GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &sub_name_max_len);
      MiVector<char> name_data(sub_name_max_len);

      // the layout qualifier order sometimes doesn't correspond
      // to the actual order inside program
      // layout(location = x) or layout(index = y)
      // request the indices via names
      for (GLint sub = 0; sub < sub_count; ++sub) {
        glGetActiveSubroutineName(program_, iface.shader_type, sub,
                                  static_cast<GLsizei>(sub_name_max_len),
                                  nullptr, name_data.data());
        std::string sub_name(name_data.data());

        GLuint sub_index =
            glGetSubroutineIndex(program_, iface.shader_type, sub_name.c_str());
        sub_indices.try_emplace(sub_name, sub_index);
      }

      for (GLint loc = 0; loc < loc_count; ++loc) {
        GLint loc_name_len;
        glGetActiveSubroutineUniformiv(program_, iface.shader_type, loc,
                                       GL_UNIFORM_NAME_LENGTH, &loc_name_len);

        name_data.resize(loc_name_len);
        glGetActiveSubroutineUniformName(program_, iface.shader_type, loc,
                                         static_cast<GLsizei>(loc_name_len),
                                         nullptr, name_data.data());
        std::string loc_name(name_data.data());

        GLint loc_index = glGetSubroutineUniformLocation(
            program_, iface.shader_type, loc_name.c_str());
        loc_indices.try_emplace(loc_name, static_cast<GLuint>(loc_index));
      }
    }
  }
}

void ProgramSubroutines::SetSubroutineToLocation(GLenum shader_type,
                                                 const std::string &sub_name,
                                                 const std::string &loc_name) {
  auto type_it = subroutines_.find(shader_type);
  if (type_it != subroutines_.end()) {
    const auto &locations = type_it->second.locations_indices;
    const auto &subroutines = type_it->second.subroutines_indices;
    auto &to_gpu = type_it->second.to_gpu;

    auto loc_it = locations.find(loc_name);
    if (loc_it != locations.end()) {
      auto sub_it = subroutines.find(sub_name);
      if (sub_it != subroutines.end()) {
        // that happens
        to_gpu[loc_it->second] = sub_it->second;
      } else {
        spdlog::warn("{}: Subroutine not found '{}'", __FUNCTION__, sub_name);
      }
    } else {
      spdlog::warn("{}: Uniform location not found '{}'", __FUNCTION__,
                   loc_name);
    }
  } else {
    spdlog::warn("{}: Shader type incorrect", __FUNCTION__);
  }
}

void ProgramSubroutines::UseSubroutines(GLenum shader_type) {
  auto sub_it = subroutines_.find(shader_type);
  if (sub_it != subroutines_.end()) {
    auto &to_gpu = sub_it->second.to_gpu;
    glUniformSubroutinesuiv(shader_type, static_cast<GLsizei>(to_gpu.size()),
                            to_gpu.data());
  } else {
    spdlog::warn("{}: Shader type incorrect", __FUNCTION__);
  }
}

}  // namespace gl