#pragma once

// deps
#include <glad/glad.h>

namespace id {

enum Enum : GLuint {
  kCamera,
  kModel,
  kTexture,
  kEnvTexture,
  kObject,
  kPointLight,
  kFxInstance,
  kTotal
};

using Camera = GLuint;
using Model = GLuint;
using Texture = GLuint;
using EnvTexture = GLuint;
using Object = GLuint;
using PointLight = GLuint;
using FxInstance = GLuint;

GLuint GenId(GLuint id_type);

}  // namespace id
