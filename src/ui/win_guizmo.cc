#include "win_guizmo.h"

// deps
#include <imgui.h>
// guizmo, after imgui
#include <ImGuizmo.h>
// local
#include "scene/camera_system.h"
#include "scene/scene.h"

namespace ui {

bool WinGuizmo::Translation(float* translation) {
  ImGuizmo::SetID(0);
  static bool use_snap = false;
  static float snap[3] = {1.0f, 1.0f, 1.0f};
  static glm::mat4 transformation{1.0f};
  float* t = &transformation[3][0];

  ImGui::Checkbox("Use Snap##guizmo_light", &use_snap);
  ImGui::SameLine();
  ImGui::InputFloat3("Snap##guizmo_light", &snap[0]);

  std::copy(translation, translation + 3, t);
  ImGuizmo::Manipulate(&camera_->view_[0][0],          //
                       &camera_->proj_[0][0],          //
                       ImGuizmo::TRANSLATE,            //
                       ImGuizmo::WORLD,                //
                       &transformation[0][0],          //
                       nullptr,                        //
                       use_snap ? &snap[0] : nullptr,  //
                       nullptr, nullptr                //
  );

  if (ImGuizmo::IsUsing()) {
    std::copy(t, t + 3, translation);
    return true;
  }
  return false;
}

bool WinGuizmo::Full(float* translation, float* euler_deg, float* scale) {
  ImGuizmo::SetID(1);
  static int gizmo_operation = ImGuizmo::TRANSLATE;
  static bool use_snap = false;
  static float snap[3] = {1.0f, 1.0f, 1.0f};
  static float transformation[16] = {};
  static glm::vec3 dummy_rotation{0.0f};

  ImGui::RadioButton("Translate##guizmo_obj", &gizmo_operation,
                     ImGuizmo::TRANSLATE);
  ImGui::SameLine();
  ImGui::RadioButton("Rotate##guizmo_obj", &gizmo_operation, ImGuizmo::ROTATE);
  ImGui::SameLine();
  ImGui::RadioButton("Scale##guizmo_obj", &gizmo_operation, ImGuizmo::SCALE);

  ImGui::Checkbox("Use Snap##guizmo_obj", &use_snap);
  ImGui::SameLine();

  switch (gizmo_operation) {
    case ImGuizmo::TRANSLATE:
      ImGui::InputFloat3("Snap##guizmo_obj", &snap[0]);
      break;
    case ImGuizmo::ROTATE:
      ImGui::InputFloat("Angle Snap##guizmo_obj", &snap[0]);
      break;
    case ImGuizmo::SCALE:
      ImGui::InputFloat("Scale Snap##guizmo_obj", &snap[0]);
      break;
  }

  ImGuizmo::RecomposeMatrixFromComponents(translation, euler_deg, scale,
                                          transformation);
  ImGuizmo::Manipulate(&camera_->view_[0][0],                              //
                       &camera_->proj_[0][0],                              //
                       static_cast<ImGuizmo::OPERATION>(gizmo_operation),  //
                       ImGuizmo::WORLD,                                    //
                       transformation,                                     //
                       nullptr,                                            //
                       use_snap ? &snap[0] : nullptr,                      //
                       nullptr,                                            //
                       nullptr                                             //
  );

  if (ImGuizmo::IsUsing()) {
    // WORLD mode when scaling reset rotation part
    if (gizmo_operation == ImGuizmo::SCALE) {
      ImGuizmo::DecomposeMatrixToComponents(transformation, translation,
                                            &dummy_rotation[0], scale);
    } else {
      ImGuizmo::DecomposeMatrixToComponents(transformation, translation,
                                            euler_deg, scale);
    }
    return true;
  }
  return false;
}

void WinGuizmo::ShowGuizmo() {
  ImGui::RadioButton("Object##guizmo", &guizmo_type_, GuizmoType::kObject);
  ImGui::SameLine();
  ImGui::RadioButton("Point Light##guizmo", &guizmo_type_,
                     GuizmoType::kPointLight);
  ImGui::SameLine();
  ImGui::RadioButton("Fx Instance##guizmo", &guizmo_type_,
                     GuizmoType::kFxInstance);
  ImGui::Separator();

  auto* object = scene_->selected_object_;
  auto* point_light = scene_->selected_point_light_;
  auto* fx_instance = scene_->selected_fx_instance_;

  switch (guizmo_type_) {
    case GuizmoType::kObject:
      if (object) {
        glm::vec3 position = object->GetPosition();
        glm::vec3 euler_deg = object->GetEulerDeg();
        glm::vec3 scale = object->GetScale();
        if (Full(&position[0], &euler_deg[0], &scale[0])) {
          object->SetPosition(position);
          object->SetEulerDeg(euler_deg);
          object->SetScale(scale);
          scene_->objects_.UpdateObject(*object);
        }
      }
      break;
    case GuizmoType::kPointLight:
      if (point_light) {
        glm::vec3& light_pos = point_light->position_;
        if (Translation(&light_pos[0])) {
          scene_->lights_.UpdatePointLight(*point_light);
        }
      }
      break;
    case GuizmoType::kFxInstance:
      if (fx_instance) {
        glm::vec4& fx_pos = fx_instance->particles.emitter_pos;
        if (Translation(&fx_pos[0])) {
          // updated each frame by ParticleSystem
        }
      }
      break;
  }
}

}  // namespace ui