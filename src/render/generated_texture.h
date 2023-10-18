#pragma once

// deps
#include <glad/glad.h>
// local
#include "opengl/texture.h"

class GeneratedTexture {
 public:
  GeneratedTexture();
  // OpenGL resources (move-only)
  GeneratedTexture(const GeneratedTexture &) = delete;
  GeneratedTexture &operator=(const GeneratedTexture &) = delete;
  GeneratedTexture(GeneratedTexture &&) noexcept = default;
  GeneratedTexture &operator=(GeneratedTexture &&) noexcept = default;

 public:
  gl::Texture3D shadow_random_angles_;
  gl::Texture2D hilbert_indices_;
};