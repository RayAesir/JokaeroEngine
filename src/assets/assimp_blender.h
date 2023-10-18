#pragma once

// Blender ../scripts/addons/io_scene_fbx/export_fbx_bin.py
// doesn't export Ambient Occlusion or Height/Bump Map
#define AI_MATKEY_FBX_BLENDER_DIFFUSE_TEXTURE \
  "$raw.DiffuseColor|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_METALLIC_TEXTURE \
  "$raw.ReflectionFactor|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_SPECULAR_TEXTURE \
  "$raw.SpecularFactor|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_ROUGHNESS_TEXTURE \
  "$raw.ShininessExponent|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_EMISSIVE_TEXTURE \
  "$raw.EmissiveColor|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_EMISSIVE_FACTOR_TEXTURE \
  "$raw.EmissiveFactor|file", aiTextureType_UNKNOWN, 0
#define AI_MATKEY_FBX_BLENDER_NORMALS_TEXTURE \
  "$raw.NormalMap|file", aiTextureType_UNKNOWN, 0
