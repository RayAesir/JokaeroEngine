#pragma once

// deps
#include <glad/glad.h>
// global
#include <chrono>

namespace gl {

class Query {
 public:
  Query(GLenum type = GL_TIME_ELAPSED);
  ~Query();
  // OpenGL resources (only-move)
  Query(const Query &) = delete;
  Query &operator=(const Query &) = delete;
  Query(Query &&o) noexcept;
  Query &operator=(Query &&o) noexcept;

 public:
  void Begin();
  void End();
  GLuint GetResult() const;
  double GetResultAsTime();

 private:
  GLuint query_;
  GLenum type_;
  bool wait_{false};
  GLuint available_{0};
  GLuint result_{0};
  std::chrono::duration<double, std::milli> ms_;
};

}  // namespace gl