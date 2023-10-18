// CPU sends GLuint64[8], here as struct
struct Material {
  uvec2 hnd_diffuse;
  uvec2 hnd_metallic;
  uvec2 hnd_roughness;
  uvec2 hnd_emissive;
  uvec2 hnd_emissive_factor;
  uvec2 hnd_normals;
  //
  uint tex_flags;
  float roughness;
  float metallic;
  float empty;
  //
  vec3 diffuse_color;
  float alpha_threshold;
  //
  vec3 emission_color;
  float emission_intensity;
};

const uint kDiffuse = 1 << 0;
const uint kMetallic = 1 << 1;
const uint kRoughness = 1 << 2;
const uint kEmissive = 1 << 3;
const uint kEmissiveFactor = 1 << 4;
const uint kNormals = 1 << 5;

layout(std430, binding = BIND_MATERIALS) STORAGE_MATERIALS
    restrict buffer StorageMaterials {
  Material sMaterials[];
};

vec4 GetDiffuse(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kDiffuse)) {
    sampler2D tex_diffuse = sampler2D(mat.hnd_diffuse);
    return texture(tex_diffuse, uv);
  } else {
    return vec4(mat.diffuse_color, mat.alpha_threshold);
  }
}

// AlphaClip only
float GetAlpha(Material mat, vec2 uv) {
  sampler2D tex_diffuse = sampler2D(mat.hnd_diffuse);
  return texture(tex_diffuse, uv).a;
}

vec3 GetNormals(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kNormals)) {
    sampler2D tex_normals = sampler2D(mat.hnd_normals);
    vec3 normal_map = texture(tex_normals, uv).xyz;
    return normalize(normal_map * 2.0 - 1.0);
  } else {
    return vec3(0.0, 0.0, 1.0);
  }
}

float GetRoughness(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kRoughness)) {
    sampler2D tex_roughness = sampler2D(mat.hnd_roughness);
    return texture(tex_roughness, uv).r;
  } else {
    return mat.roughness;
  }
}

float GetMetallic(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kMetallic)) {
    sampler2D tex_metallic = sampler2D(mat.hnd_metallic);
    return texture(tex_metallic, uv).r;
  } else {
    return mat.metallic;
  }
}

vec3 GetEmission(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kEmissive)) {
    sampler2D tex_emissive = sampler2D(mat.hnd_emissive);
    return texture(tex_emissive, uv).rgb;
  } else {
    return mat.emission_color;
  }
}

float GetEmissionFactor(Material mat, vec2 uv) {
  if (bool(mat.tex_flags & kEmissiveFactor)) {
    sampler2D tex_emissive_factor = sampler2D(mat.hnd_emissive_factor);
    return texture(tex_emissive_factor, uv).r;
  } else {
    return mat.emission_intensity;
  }
}
