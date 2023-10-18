#pragma once

// local
#include "assets/env_texture_manager.h"
#include "assets/model_manager.h"
#include "assets/particle_manager.h"
#include "assets/texture_manager.h"

class Assets {
 public:
  Assets(TextureManager& textures,    //
         EnvTextureManager& env_tex,  //
         ModelManager& models,        //
         ParticleManager& particles   //
  );
  using Inject = Assets(TextureManager&, EnvTextureManager&, ModelManager&,
                        ParticleManager&);

 public:
  TextureManager& textures_;
  EnvTextureManager& env_tex_;
  ModelManager& models_;
  ParticleManager& particles_;
};
