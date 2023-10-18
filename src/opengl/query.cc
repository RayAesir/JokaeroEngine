#include "query.h"

namespace gl {

Query::Query(GLenum type) : type_(type) { glCreateQueries(type, 1, &query_); }

Query::~Query() { glDeleteQueries(1, &query_); }

Query::Query(Query &&o) noexcept : query_(std::exchange(o.query_, 0)){};

Query &Query::operator=(Query &&o) noexcept {
  if (&o == this) return *this;
  if (query_) glDeleteQueries(1, &query_);

  query_ = std::exchange(o.query_, 0);
  return *this;
}

void Query::Begin() {
  if (!wait_) {
    glBeginQuery(type_, query_);
  }
}

void Query::End() {
  if (!wait_) {
    glEndQuery(type_);
    wait_ = true;
  }
  // GL_QUERY_RESULT request is synchronous
  // to avoid the CPU stall use GL_QUERY_RESULT_AVAILABLE
  glGetQueryObjectuiv(query_, GL_QUERY_RESULT_AVAILABLE, &available_);
  if (available_) {
    glGetQueryObjectuiv(query_, GL_QUERY_RESULT, &result_);
    wait_ = false;
  }
}

GLuint Query::GetResult() const { return result_; }

double Query::GetResultAsTime() {
  ms_ = std::chrono::nanoseconds(result_);
  // reset a value to get the total time of pipeline stages
  // stages can be enabled/disabled
  result_ = 0;
  return ms_.count();
}

}  // namespace gl