#include "win_scene.h"

// deps
#include <imgui.h>
// local
#include "app/parameters.h"
#include "assets/assets.h"
#include "imgui_toggle/imgui_toggle.h"
#include "imguizmo.quat/imGuIZMOquat.h"
#include "options.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "ui/base_components.h"
#include "utils/enums.h"

namespace ui {

void WinScene::ShowScene() {
  auto& lighting = scene_->lighting_;
  const auto& count = *renderer_->atomic_counters_;

  ImGui::Text("Indirect CMD (count, offset):");
  for (GLuint i = 0; i < MeshType::kTotal; ++i) {
    GLsizei drawcount = renderer_->indirect_drawcount_[i];
    GLuint64 offset = renderer_->indirect_offsets_[i];
    ImGui::Text("[%s]: %d, %llu", NamedEnum<MeshType::Enum>::ToStr()[i],
                drawcount, offset);
  }

  ImGui::Text("Instances:");
  ImGui::Text("Shadows: [Direct: %u, Point: %u]",
              count.frustum_instances_direct_shadows,  //
              count.frustum_instances_point_shadows    //
  );
  ImGui::Text("View: [Total: %u, Frustum: %u, Occlusion: %u]",
              scene_->instance_count_,        //
              count.frustum_instances_view,   //
              count.occlusion_instances_view  //
  );
  ImGui::Text("Deleted: %u", scene_->objects_.GetInstanceDeleted());

  ImGui::Text("Point Lights: [Total: %u, Light: %u, Shadow: %u]",
              scene_->point_light_count_,  //
              count.frustum_point_lights,  //
              count.frustum_point_shadows  //
  );
  ImGui::Text("Deleted: %u", scene_->lights_.GetPointLightDeleted());

  ImGui::Separator();
  ImGui::Text("Global Lighting:");
  static float gizmo_size = 128.0f * app::glfw.window_scale.x;
  ImGui::gizmo3D("##scene_gizmo", lighting.direction, gizmo_size,
                 imguiGizmo::modeDirection);
  if (ImGui::SliderFloat3("Direction##scene", &lighting.direction[0], -1.0f,
                          1.0f, "%.2f")) {
    lighting.direction = glm::normalize(lighting.direction);
  }

  ImGui::ColorEdit3("Diffuse##scene", &lighting.diffuse[0]);
  ImGui::SliderFloat("Diffuse Intensity##scene", &lighting.diffuse.w, 0.01f,
                     5.0f, "%.2f");

  ImGui::ColorEdit3("Ambient##scene", &lighting.ambient[0]);
  ImGui::SliderFloat("Ambient Intensity##scene", &lighting.ambient.w, 0.01f,
                     5.0f, "%.2f");

  ImGui::ColorEdit3("Skybox##scene", &lighting.skybox[0]);
  ImGui::SliderFloat("Skybox Intensity##scene", &lighting.skybox.w, 0.01f, 5.0f,
                     "%.2f");

  if (ImGui::Button("Create Point Light##light")) {
    scene_->CreatePointLightAtCamera();
  }
}

void WinScene::ShowActiveCamera(MainCamera& camera) {
  glm::vec3 front = camera.Front();
  ImGui::Text("Name: %s [%llu]", camera.GetName().c_str(), camera.GetId());
  ImGui::Text("Position [x: %.3f, y: %.3f, z: %.3f]", camera.position_.x,
              camera.position_.y, camera.position_.z);
  ImGui::Text("Direction [x: %.3f, y: %.3f, z: %.3f]", front.x, front.y,
              front.z);
  ImGui::Text("Velocity [x: %.3f, y: %.3f, z: %.3f]", camera.velocity_.x,
              camera.velocity_.y, camera.velocity_.z);
  ImGui::Text("Speed: %.3f", camera.speed_);

  if (ImGui::Button("FPS Camera Preset##cam")) {
    camera.fps_style_ = true;
    camera.remove_roll_ = true;
    camera.uniform_speed_ = true;
    camera.head_bobbing_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Free Camera Preset##cam")) {
    camera.fps_style_ = false;
    camera.remove_roll_ = false;
    camera.uniform_speed_ = false;
    camera.head_bobbing_ = false;
  }

  ImGui::Separator();

  if (camera.uniform_speed_) {
    if (ImGui::SliderFloat("Speed##cam", &camera.axis_speed_.x, 0.1f, 50.0f,
                           "%.1f")) {
      camera.axis_speed_ = glm::vec3(camera.axis_speed_.x);
    }
  } else {
    ImGui::SliderFloat("Longitudinal Speed Z##cam", &camera.axis_speed_.z, 0.1f,
                       50.0f, "%.1f");
    ImGui::SliderFloat("Vertical Speed Y##cam", &camera.axis_speed_.y, 0.1f,
                       50.0f, "%.1f");
    ImGui::SliderFloat("Horizontal Speed X##cam", &camera.axis_speed_.x, 0.1f,
                       50.0f, "%.1f");
  }

  ImGui::SliderFloat("Max Speed##cam", &camera.max_speed_, 1.0f, 50.0f, "%.1f");
  ImGui::SliderFloat("Acceleration##cam", &camera.acceleration_, 0.1f, 10.0f,
                     "%.1f");
  ImGui::SliderFloat("Surface Friction##cam", &camera.surface_friction_, 0.1f,
                     10.0f, "%.1f");
  ImGui::SliderFloat("Velocity Friction##cam", &camera.velocity_friction_, 0.1f,
                     10.0f, "%.1f");
  ImGui::SliderAngle("Roll Speed##cam", &camera.roll_speed_rad_, 1.0f, 180.0f,
                     "%1.0f");

  ImGui::Separator();

  ImGui::Text("Horizontal FOV: %.3f", camera.FovHorizontalDeg());
  int req = 0;
  req += ImGui::SliderAngle("Vertical FOV##cam", &camera.fov_,
                            global::kMinFovDeg, global::kMaxFovDeg, "%.1f");
  req += ImGui::SliderFloat("Z Near##cam", &camera.z_near_, 0.01f, 100.0f);
  req += ImGui::SliderFloat("Z Far##cam", &camera.z_far_, 100.0f, 1000.0f);
  if (req) {
    camera.update_clusters_ = true;
  }

  ImGui::Separator();

  if (ImGui::CollapsingHeader("Settings##cam")) {
    static int flags = ImGuiToggleFlags_Animated;
    ImGui::Toggle("FPS Style##cam", &camera.fps_style_, flags);
    ImGui::SameLine();
    ImGui::Toggle("Remove Roll##cam", &camera.remove_roll_, flags);

    ImGui::Toggle("Uniform Speed##cam", &camera.uniform_speed_, flags);
    ImGui::SameLine();
    ImGui::Toggle("Head Bobbing##cam", &camera.head_bobbing_, flags);
  }

  if (ImGui::CollapsingHeader("Head Bobbing##cam")) {
    ImGui::Text("Travel Dist [%.1f]", camera.travel_dist_);
    ImGui::Text("Fade Mod [%.3f]", camera.bob_fade_mod_);
    ImGui::SliderFloat("Fade Base", &camera.bob_fade_base_, 0.01f, 1.0f,
                       "%.2f");
    ImGui::SliderFloat("Fade Min", &camera.bob_fade_min_, 0.01f, 1.0f, "%.2f");
    ImGui::SliderFloat("Bob Mod X", &camera.bob_mod_.x, 0.01f, 1.0f, "%.2f");
    ImGui::SliderFloat("Bob Mod Y", &camera.bob_mod_.y, 0.01f, 1.0f, "%.2f");
  }
}

void WinScene::ShowActivePointLight(PointLight& light) {
  int req = 0;
  ImGui::Text("Selected Light: point_light %llu", light.GetId());
  ImGui::Separator();
  req += ImGui::InputFloat3("Position##light", &light.position_[0]);
  ImGui::Separator();
  req +=
      ImGui::SliderFloat("Radius##light", &light.radius_, 0.1f, 20.0f, "%.1f");
  req += ImGui::ColorEdit3("Diffuse##light", &light.diffuse_[0]);
  req += ImGui::SliderFloat("Intensity##light", &light.intensity_, 0.1f, 10.0f,
                            "%.1f");
  bool cast_shadows = light.IsCastShadows();
  if (ImGui::Checkbox("Cast Shadows##light", &cast_shadows)) {
    light.SetProps(Props::kCastShadows, cast_shadows);
    ++req;
  }

  if (req) {
    scene_->lights_.UpdatePointLight(light);
  }

  if (ImGui::Button("Delete##light")) {
    scene_->DeleteSelectedPointLight();
  }
}

void WinScene::ShowActiveObject(Model& model, Object& object) {
  int req = 0;
  glm::vec3 position = object.GetPosition();
  glm::vec3 euler_rad = object.GetEuler();
  glm::vec3 scale = object.GetScale();

  ImGui::Text("Selected Object: %s [%llu]", model.GetName().c_str(),
              object.GetId());
  static constexpr const char* bool_to_str[2]{"No", "Yes"};
  ImGui::Text("Animated: %s", bool_to_str[object.IsAnimated()]);
  ImGui::Separator();
  if (ImGui::InputFloat3("Position##obj", &position[0])) {
    object.SetPosition(position);
    ++req;
  }
  if (ImGui::SliderAngle("Rotation X##obj", &euler_rad.x)) {
    object.SetEuler(euler_rad);
    ++req;
  }
  if (ImGui::SliderAngle("Rotation Y##obj", &euler_rad.y)) {
    object.SetEuler(euler_rad);
    ++req;
  }
  if (ImGui::SliderAngle("Rotation Z##obj", &euler_rad.z)) {
    object.SetEuler(euler_rad);
    ++req;
  }
  if (ImGui::InputFloat3("Scale##obj", &scale[0])) {
    object.SetScale(scale);
    ++req;
  }

  if (req) {
    scene_->objects_.UpdateObject(object);
  }
}

void WinScene::ShowActiveObjectMaterials(Model& model) {
  auto& meshes = model.meshes_;

  static size_t current = 0;
  if (current >= meshes.size()) current = 0;

  if (meshes.size()) {
    const char* preview_mode = meshes[current].GetName().c_str();
    if (ImGui::BeginCombo("##obj_mat", preview_mode)) {
      for (size_t i = 0; i < meshes.size(); ++i) {
        const bool is_selected = (current == i);
        if (ImGui::Selectable(meshes[i].GetName().c_str(), is_selected)) {
          current = i;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    auto& mesh = meshes[current];
    auto& mat = mesh.material_;

    ImGui::Text("Mesh Type: %s",
                NamedEnum<MeshType::Enum>::ToStr()[mesh.GetType()]);
    ImGui::Separator();

    if (mat.tex_flags & (1U << TextureType::kDiffuse)) {
      ImGui::Text("Diffuse Color: Texture");
    } else {
      ImGui::ColorEdit3("Diffuse Color##obj",
                        &mat.diffuse_color_alpha_threshold[0]);
    }

    ImGui::SliderFloat("Alpha Threshold##obj",
                       &mat.diffuse_color_alpha_threshold.w, 0.0f, 1.0f,
                       "%.2f");

    if (mat.tex_flags & (1U << TextureType::kMetallic)) {
      ImGui::Text("PBR Metallic: Texture");
    } else {
      ImGui::SliderFloat("PBR Metallic##obj", &mat.metallic, 0.0f, 1.0f,
                         "%.2f");
    }

    if (mat.tex_flags & (1U << TextureType::kRoughness)) {
      ImGui::Text("PBR Roughness: Texture");
    } else {
      ImGui::SliderFloat("PBR Roughness##obj", &mat.roughness, 0.0f, 1.0f,
                         "%.2f");
    }

    if (mat.tex_flags & (1U << TextureType::kEmissive)) {
      ImGui::Text("Emission Color: Texture");
    } else {
      ImGui::ColorEdit3("Emission Color##obj",
                        &mat.emission_color_emission_intensity[0]);
    }

    if (mat.tex_flags & (1U << TextureType::kEmissiveFactor)) {
      ImGui::Text("Emission Intensity: Texture");
    } else {
      ImGui::SliderFloat("Emission Intensity##obj",
                         &mat.emission_color_emission_intensity.w, 0.0f, 1.0f,
                         "%.2f");
    }

    bool use_normals_tex = mat.tex_flags & (1U << TextureType::kNormals);
    ImGui::Text("Normals: %s", (use_normals_tex) ? "Texture" : "Mesh Faces");
    ImGui::Separator();

    if (ImGui::Button("Update Material##obj")) {
      assets_->models_.UpdateMeshMaterial(mesh);
    }
  }
}

void WinScene::ShowActiveObjectAnimations(Model& model,
                                          AnimatedObject& object) {
  const auto& animations = model.GetAnimations();
  static size_t current = 0;
  if (current >= animations.size()) current = 0;

  if (animations.size()) {
    const char* preview_mode = animations[current].GetName().c_str();
    if (ImGui::BeginCombo("##obj_anim", preview_mode)) {
      for (size_t i = 0; i < animations.size(); ++i) {
        const bool is_selected = (current == i);
        if (ImGui::Selectable(animations[i].GetName().c_str(), is_selected)) {
          current = i;
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    auto& anim = animations[current];

    ImGui::Text("Total Bones: %llu", anim.GetBones().size());
    ImGui::Text("Duration: %.3f", anim.GetDuration());
    ImGui::Text("Ticks Per Second: %d", anim.GetTicksPerSecond());

    if (ImGui::Button("Set Animation##obj")) {
      object.SetAnimation(model.GetAnimationByIndex(current));
    }
  }
}

void WinScene::ShowContextObject(Model& model, Object& object) {
  if (ImGui::BeginTabBar("TabBarObject##ctx")) {
    if (ImGui::BeginTabItem("Object##ctx")) {
      ShowActiveObject(model, object);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Materials##ctx")) {
      ShowActiveObjectMaterials(model);
      ImGui::EndTabItem();
    }
    if (object.IsAnimated()) {
      if (ImGui::BeginTabItem("Animations##ctx")) {
        auto& animated = *static_cast<AnimatedObject*>(&object);
        ShowActiveObjectAnimations(model, animated);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
}

void WinScene::ShowFxInstance(FxInstance& fx_instance) {
  const auto* fx = fx_instance.fx;
  auto& particles = fx_instance.particles;

  ImGui::Text("%s: %llu", fx->name.c_str(), fx_instance.id);

  ImGui::InputFloat3("Emitter Position##fx", &particles.emitter_pos[0]);

  ImGui::SliderFloat2("Particle Size Min/Max##fx",
                      &particles.particle_size_min_max[0], 0.01f, 5.0f, "%.2f");
  ImGui::ColorEdit3("Color##fx", &particles.color[0]);
  ImGui::SliderFloat("Should Keep Color##fx", &particles.should_keep_color,
                     0.0f, 1.0f, "%1.0f");

  if (ImGui::Button("Delete##fx")) {
    scene_->DeleteSelectedParticles();
  }
}

void WinScene::Headers() {
  auto* object = scene_->selected_object_;
  auto* point_light = scene_->selected_point_light_;
  auto* fx_instance = scene_->selected_fx_instance_;

  if (ImGui::CollapsingHeader("Scene")) {
    ShowScene();
    ImGui::NewLine();
  }
  if (ImGui::CollapsingHeader("Active Camera")) {
    ShowActiveCamera(*camera_);
    ImGui::NewLine();
  }
  if (point_light) {
    if (ImGui::CollapsingHeader("Active Point Light")) {
      ShowActivePointLight(*point_light);
      ImGui::NewLine();
    }
  }
  if (object) {
    if (ImGui::CollapsingHeader("Active Object")) {
      ShowContextObject(object->GetModel(), *object);
      ImGui::Separator();
      if (ImGui::Button("Delete##obj")) {
        scene_->DeleteSelectedObject();
      }
      ImGui::NewLine();
    }
  }
  if (fx_instance) {
    if (ImGui::CollapsingHeader("FX Instance (Particles)")) {
      ShowFxInstance(*fx_instance);
      ImGui::NewLine();
    }
  }
}

}  // namespace ui