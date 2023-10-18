#pragma once

// deps
#include <glad/glad.h>

namespace gl {

// Completion of commands may be determined either by calling
// glFinish() or by calling glFenceSync() and glClientWaitSync()
// The second method doesn't required a round trip to the GL server and may be
// more efficient (recommended way from OpenGL v3.2)
class Sync {
 public:
  // reversed order of glFenceSync() and glClientWaitSync()
  // one thread creates other waits
  void BeginMt() const;
  void EndMt() const;
  void FenceSt() const;

 private:
  mutable GLsync sync_{nullptr};
};

}  // namespace gl