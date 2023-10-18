#include "id_generator.h"

// global
#include <array>
#include <atomic>

namespace id {

namespace {
std::array<std::atomic<GLuint>, kTotal> gIds{};
}

GLuint GenId(GLuint id_type) { return gIds[id_type]++; }

}  // namespace id
