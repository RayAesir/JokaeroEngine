#pragma once

// global
#include <string>

namespace glsl {

// WARP_SIZE (NVIDIA 32, AMD 64) can be runtime only
// load before all other shader files
void Header(const std::string &filename);

}