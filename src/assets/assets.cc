#include "assets.h"

Assets::Assets(TextureManager& textures,    //
               EnvTextureManager& env_tex,  //
               ModelManager& models,        //
               ParticleManager& particles   //
               )
    : textures_(textures),
      env_tex_(env_tex),
      models_(models),
      particles_(particles) {}