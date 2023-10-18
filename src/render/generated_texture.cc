#include "generated_texture.h"

// local
#include "global.h"
#include "math/random.h"

GeneratedTexture::GeneratedTexture() {
  // random angles for shadows
  shadow_random_angles_.SetStorage3D(1, GL_RG32F, global::kRandomAnglesSize,
                                     global::kRandomAnglesSize,
                                     global::kRandomAnglesSize);
  shadow_random_angles_.SetFilter(GL_NEAREST, GL_NEAREST);
  shadow_random_angles_.SetWrap3D(GL_REPEAT);

  MiVector<glm::vec2> shadow_random_angles;
  rnd::GenerateShadowAngles(global::kRandomAnglesSize, shadow_random_angles);
  shadow_random_angles_.SubImage3D(
      global::kRandomAnglesSize, global::kRandomAnglesSize,
      global::kRandomAnglesSize, GL_RG, GL_FLOAT, shadow_random_angles.data());

  // spatial noise for GTAO
  // integer so GL_NEAREST
  hilbert_indices_.SetStorage2D(1, GL_R16UI, global::kHilbertSize,
                                global::kHilbertSize);
  hilbert_indices_.SetFilter(GL_NEAREST, GL_NEAREST);
  hilbert_indices_.SetWrap2D(GL_REPEAT);

  MiVector<uint16_t> hilbert_indices;
  rnd::GenerateHilbertIndices(static_cast<uint16_t>(global::kHilbertSize),
                              hilbert_indices);
  hilbert_indices_.SubImage2D(global::kHilbertSize, global::kHilbertSize,
                              GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                              hilbert_indices.data());
}
