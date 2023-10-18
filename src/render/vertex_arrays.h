#pragma once

// local
#include "global.h"
#include "opengl/vertex_array.h"
// fwd
namespace gl {
class VertexBuffers;
}

struct VertexArrays {
  VertexArrays(gl::VertexBuffers& gen_mesh_internal,  //
               gl::VertexBuffers& mesh_static,        //
               gl::VertexBuffers& mesh_skinned,       //
               GLuint instance_attributes,            //
               GLuint instance_matrix_mvp);
  // Debug Boxes
  gl::VertexArray genmesh_pos_inst;
  // Point shadowpass
  gl::VertexArray pos_inst;
  gl::VertexArray pos_skin_inst;
  gl::VertexArray pos_tex_inst;
  gl::VertexArray pos_tex_skin_inst;
  // Prepass and Direct shadowpass
  gl::VertexArray pos_inst_mvp;
  gl::VertexArray pos_skin_inst_mvp;
  gl::VertexArray pos_tex_inst_mvp;
  gl::VertexArray pos_tex_skin_inst_mvp;
  // View renderpass
  gl::VertexArray pos_norm_tex_inst_mvp;
  gl::VertexArray pos_norm_tex_skin_inst_mvp;

  // comfy access
  const gl::VertexArray* prepass[MeshType::kTotal]{
      &pos_inst_mvp,           //
      &pos_skin_inst_mvp,      //
      &pos_tex_inst_mvp,       //
      &pos_tex_skin_inst_mvp,  //
      &pos_inst_mvp,           //
      &pos_skin_inst_mvp       //
  };
  const gl::VertexArray* direct_shadowpass[MeshType::kNoAlphaBlend]{
      &pos_inst_mvp,          //
      &pos_skin_inst_mvp,     //
      &pos_tex_inst_mvp,      //
      &pos_tex_skin_inst_mvp  //
  };
  const gl::VertexArray* point_shadowpass[MeshType::kNoAlphaBlend]{
      &pos_inst,          //
      &pos_skin_inst,     //
      &pos_tex_inst,      //
      &pos_tex_skin_inst  //
  };
  const gl::VertexArray* viewpass[MeshType::kTotal]{
      &pos_norm_tex_inst_mvp,       //
      &pos_norm_tex_skin_inst_mvp,  //
      &pos_norm_tex_inst_mvp,       //
      &pos_norm_tex_skin_inst_mvp,  //
      &pos_norm_tex_inst_mvp,       //
      &pos_norm_tex_skin_inst_mvp   //
  };
};
