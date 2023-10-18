#include "texture.h"

// deps
#include <spdlog/spdlog.h>
#include <stb_image.h>
// local
#include "assets/texture_manager.h"
#include "options.h"

GLsizei GetMipMapLevel(int width, int height) {
  GLsizei level = 1;
  if (width == height) {
    GLsizei min_size = width;
    while (min_size != 1) {
      min_size /= 2;
      ++level;
    }
  } else {
    GLsizei min_width = width;
    GLsizei min_height = width;
    while (min_width != 1 && min_height != 1) {
      min_width /= 2;
      min_height /= 2;
      ++level;
    }
  }
  return level;
};

Image::Image(const std::string &path) {
  data = stbi_load(path.c_str(), &width, &height, &num_of_channels, 0);
  if (data) {
    success = true;
  } else {
    spdlog::error("{}: Failed to load '{}', reason '{}'", __FUNCTION__, path,
                  stbi_failure_reason());
    stbi_image_free(data);
    success = false;
  }
}

Image::~Image() {
  if (data) {
    stbi_image_free(data);
  }
}

ImageHdr::ImageHdr(const std::string &path) {
  data = stbi_loadf(path.c_str(), &width, &height, &num_of_channels, 0);
  if (data) {
    success = true;
  } else {
    spdlog::error("{}: Failed to load '{}', reason '{}'", __FUNCTION__, path,
                  stbi_failure_reason());
    stbi_image_free(data);
    success = false;
  }
}

ImageHdr::~ImageHdr() {
  if (data) {
    stbi_image_free(data);
  }
}

Texture::Texture(const Image &img, TextureType::Enum type) {
  static constexpr bool kGammaCorrection[TextureType::kTotal]{
      true,   // color
      false,  // linear
      false,  // linear
      true,   // color
      false,  // linear
      false,  // linear
  };
  bool gamma_correction = kGammaCorrection[type];
  // setup OpenGL texture object
  GLenum internal_format = GL_RGB8;
  GLenum data_format = GL_RGB;
  GLint wrap_method = GL_REPEAT;
  switch (img.num_of_channels) {
    case 1:
      internal_format = GL_R8;
      data_format = GL_RED;
      wrap_method = GL_REPEAT;
      break;
    case 2:
      internal_format = GL_RG8;
      data_format = GL_RG;
      wrap_method = GL_REPEAT;
      break;
    case 3:
      internal_format = gamma_correction ? GL_SRGB8 : GL_RGB8;
      data_format = GL_RGB;
      wrap_method = GL_REPEAT;
      break;
    case 4:
      internal_format = gamma_correction ? GL_SRGB8_ALPHA8 : GL_RGBA8;
      data_format = GL_RGBA;
      // to prevent colored border always use GL_CLAMP_TO_EDGE with alpha
      wrap_method = GL_CLAMP_TO_EDGE;
      break;
    default:
      break;
  }

  // storage part
  GLsizei levels = GetMipMapLevel(img.width, img.height);
  tbo_.SetStorage2D(levels, internal_format, img.width, img.height);
  tbo_.SubImage2D(img.width, img.height, data_format, GL_UNSIGNED_BYTE,
                  img.data);
  tbo_.GenerateMipMap();
  // sampler part
  // model's textures will be overwritten by bindless texture sampler
  tbo_.SetWrap2D(wrap_method);
  tbo_.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  tbo_.SetAnisotropy(16.0f);
  tbo_.SetLoadBias(0.0f);
}

Texture::Texture(const ImageHdr &img) {
  constexpr GLenum internal_format = GL_RGB16F;
  constexpr GLenum data_format = GL_RGB;
  constexpr GLint wrap_method = GL_CLAMP_TO_EDGE;

  // storage part
  GLsizei levels = GetMipMapLevel(img.width, img.height);
  tbo_.SetStorage2D(levels, internal_format, img.width, img.height);
  tbo_.SubImage2D(img.width, img.height, data_format, GL_FLOAT, img.data);
  tbo_.GenerateMipMap();
  // sampler part
  tbo_.SetWrap2D(wrap_method);
  tbo_.SetFilter(GL_LINEAR, GL_LINEAR);
}

SmartTexture::SmartTexture(const Image &img, TextureType::Enum type,
                           TextureManager &manager)
    : Texture(img, type), id_(id::GenId(id::kTexture)), manager_(manager) {}

SmartTexture::~SmartTexture() { manager_.RemoveTextureMt(id_, handler_); }

id::Texture SmartTexture::GetId() const { return id_; }

GLuint64 SmartTexture::GetHandler() const { return handler_; }

void SmartTexture::SetHandler(GLuint64 handler) { handler_ = handler; }

EnvTexture::EnvTexture() : id_(id::GenId(id::kEnvTexture)) {
  environment_tbo_.SetStorage2D(opt::lighting.prefilter_max_level, GL_RGB16F,
                                opt::lighting.environment_map_size,
                                opt::lighting.environment_map_size);
  environment_tbo_.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  environment_tbo_.SetWrap3D(GL_CLAMP_TO_EDGE);

  irradiance_tbo_.SetStorage2D(1, GL_RGB16F, opt::lighting.irradiance_map_size,
                               opt::lighting.irradiance_map_size);
  irradiance_tbo_.SetFilter(GL_LINEAR, GL_LINEAR);
  irradiance_tbo_.SetWrap3D(GL_CLAMP_TO_EDGE);

  prefiltered_tbo_.SetStorage2D(opt::lighting.prefilter_max_level, GL_RGB16F,
                                opt::lighting.prefilter_map_size,
                                opt::lighting.prefilter_map_size);
  prefiltered_tbo_.SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
  prefiltered_tbo_.SetWrap3D(GL_CLAMP_TO_EDGE);
}

id::EnvTexture EnvTexture::GetId() const { return id_; }