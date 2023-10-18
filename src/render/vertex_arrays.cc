#include "vertex_arrays.h"

VertexArrays::VertexArrays(gl::VertexBuffers& gen_mesh_internal,  //
                           gl::VertexBuffers& mesh_static,        //
                           gl::VertexBuffers& mesh_skinned,       //
                           GLuint instance_attributes,            //
                           GLuint instance_matrix_mvp) {
  genmesh_pos_inst.SetAttributes({
      &kPositionAttr,  //
      &kInstanceAttr   //
  });
  genmesh_pos_inst.SetVertexBuffers(gen_mesh_internal);
  genmesh_pos_inst.SetBuffer(&kInstanceAttr, instance_attributes);
  genmesh_pos_inst.AttachBuffers();

  pos_inst.SetAttributes({
      &kPositionAttr,  //
      &kInstanceAttr   //
  });
  pos_inst.SetVertexBuffers(mesh_static);
  pos_inst.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_inst.AttachBuffers();

  pos_skin_inst.SetAttributes({
      &kPositionAttr,  //
      &kBonesAttr,     //
      &kWeightsAttr,   //
      &kInstanceAttr   //
  });
  pos_skin_inst.SetVertexBuffers(mesh_skinned);
  pos_skin_inst.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_skin_inst.AttachBuffers();

  pos_tex_inst.SetAttributes({
      &kPositionAttr,   //
      &kTexCoordsAttr,  //
      &kInstanceAttr    //
  });
  pos_tex_inst.SetVertexBuffers(mesh_static);
  pos_tex_inst.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_tex_inst.AttachBuffers();

  pos_tex_skin_inst.SetAttributes({
      &kPositionAttr,   //
      &kTexCoordsAttr,  //
      &kBonesAttr,      //
      &kWeightsAttr,    //
      &kInstanceAttr    //
  });
  pos_tex_skin_inst.SetVertexBuffers(mesh_skinned);
  pos_tex_skin_inst.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_tex_skin_inst.AttachBuffers();

  pos_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_inst_mvp.SetVertexBuffers(mesh_static);
  pos_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_inst_mvp.AttachBuffers();

  pos_skin_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kBonesAttr,      //
      &kWeightsAttr,    //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_skin_inst_mvp.SetVertexBuffers(mesh_skinned);
  pos_skin_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_skin_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_skin_inst_mvp.AttachBuffers();

  pos_tex_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kTexCoordsAttr,  //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_tex_inst_mvp.SetVertexBuffers(mesh_static);
  pos_tex_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_tex_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_tex_inst_mvp.AttachBuffers();

  pos_tex_skin_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kTexCoordsAttr,  //
      &kBonesAttr,      //
      &kWeightsAttr,    //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_tex_skin_inst_mvp.SetVertexBuffers(mesh_skinned);
  pos_tex_skin_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_tex_skin_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_tex_skin_inst_mvp.AttachBuffers();

  pos_norm_tex_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kNormalAttr,     //
      &kTangentAttr,    //
      &kBitangentAttr,  //
      &kTexCoordsAttr,  //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_norm_tex_inst_mvp.SetVertexBuffers(mesh_static);
  pos_norm_tex_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_norm_tex_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_norm_tex_inst_mvp.AttachBuffers();

  pos_norm_tex_skin_inst_mvp.SetAttributes({
      &kPositionAttr,   //
      &kNormalAttr,     //
      &kTangentAttr,    //
      &kBitangentAttr,  //
      &kTexCoordsAttr,  //
      &kBonesAttr,      //
      &kWeightsAttr,    //
      &kInstanceAttr,   //
      &kInstanceMatrix  //
  });
  pos_norm_tex_skin_inst_mvp.SetVertexBuffers(mesh_skinned);
  pos_norm_tex_skin_inst_mvp.SetBuffer(&kInstanceAttr, instance_attributes);
  pos_norm_tex_skin_inst_mvp.SetBuffer(&kInstanceMatrix, instance_matrix_mvp);
  pos_norm_tex_skin_inst_mvp.AttachBuffers();

  // debug
  genmesh_pos_inst.PrintVaoInfo();

  pos_inst.PrintVaoInfo();
  pos_skin_inst.PrintVaoInfo();
  pos_tex_inst.PrintVaoInfo();
  pos_tex_skin_inst.PrintVaoInfo();

  pos_inst_mvp.PrintVaoInfo();
  pos_skin_inst_mvp.PrintVaoInfo();
  pos_tex_inst_mvp.PrintVaoInfo();
  pos_tex_skin_inst_mvp.PrintVaoInfo();

  pos_norm_tex_inst_mvp.PrintVaoInfo();
  pos_norm_tex_skin_inst_mvp.PrintVaoInfo();
}