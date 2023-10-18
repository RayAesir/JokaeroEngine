#include "fence_sync.h"

namespace gl {

void Sync::BeginMt() const {
  if (sync_ != nullptr) {
    // actually glFenceSync() is enough
    // bool finished = false;
    // // faster than glGetSynciv(), with timeout 0
    // finished = (glClientWaitSync(sync_, GL_SYNC_FLUSH_COMMANDS_BIT, 0) ==
    //             GL_ALREADY_SIGNALED);
    glDeleteSync(sync_);
    sync_ = nullptr;
  }
}

void Sync::EndMt() const {
  if (sync_ == nullptr) {
    sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }
}

void Sync::FenceSt() const {
  // lock the buffer
  sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  // waiting for buffer, stall CPU
  GLenum finished = GL_UNSIGNALED;
  while (finished != GL_ALREADY_SIGNALED &&
         finished != GL_CONDITION_SATISFIED) {
    finished = glClientWaitSync(sync_, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
  }
  glDeleteSync(sync_);
}

}  // namespace gl