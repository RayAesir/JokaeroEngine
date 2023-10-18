#pragma once

// deps
#include <glad/glad.h>

namespace mem {

enum Type : int { kBuffer, kVertexBuffer, kTexture, kTotal };

enum Read : GLuint64 { kKB = 1024, kMB = kKB * 1024 };

void Add(Type type, const void* obj, GLuint64 bytes);
void Erase(Type type, const void* obj);
void Move(Type type, const void* other_obj, const void* this_obj);

// helper for Textures, mipmaps count
GLuint64 CalcTexBytes2D(GLenum format, GLsizei level, GLuint64 size);
GLuint64 CalcTexBytes3D(GLenum format, GLsizei level, GLuint64 size);

GLuint64 MemoryUsed(Type type, GLuint64 read);

}  // namespace mem