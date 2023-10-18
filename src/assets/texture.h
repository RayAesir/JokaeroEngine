#pragma once

// global
#include <string>
// local
#include "global.h"
#include "id_generator.h"
#include "opengl/texture.h"
// fwd
class TextureManager;

GLsizei GetMipMapLevel(int width, int height);

class Image {
 public:
  Image() = default;
  Image(const std::string &path);
  ~Image();

 public:
  int width;
  int height;
  int num_of_channels;
  unsigned char *data;
  bool success;
};

class ImageHdr {
 public:
  ImageHdr() = default;
  ImageHdr(const std::string &path);
  ~ImageHdr();

 public:
  int width;
  int height;
  int num_of_channels;
  float *data;
  bool success;
};

struct TextureType {
  enum Enum : GLuint {
    kDiffuse,
    kMetallic,
    kRoughness,
    kEmissive,
    kEmissiveFactor,
    kNormals,
    kTotal
  };
};

// simple 2D texture
class Texture {
 public:
  Texture(const Image &img, TextureType::Enum type);
  // linear space, no gamma correction
  Texture(const ImageHdr &img);

 public:
  gl::Texture2D tbo_;

  operator GLuint() const { return tbo_; }
};

// can be shared between multiple objects
class SmartTexture : public Texture {
 public:
  SmartTexture(const Image &img, TextureType::Enum type,
               TextureManager &manager);
  ~SmartTexture();

 public:
  id::Texture GetId() const;
  GLuint64 GetHandler() const;
  void SetHandler(GLuint64 handler);

 private:
  id::Texture id_{0};
  GLuint64 handler_{0};
  // the destructor deletes the record
  TextureManager &manager_;
};

// need to be rendered
class EnvTexture {
 public:
  EnvTexture();

 public:
  gl::TextureCubeMap environment_tbo_;
  gl::TextureCubeMap irradiance_tbo_;
  gl::TextureCubeMap prefiltered_tbo_;

  id::EnvTexture GetId() const;

 private:
  id::EnvTexture id_{0};
};
