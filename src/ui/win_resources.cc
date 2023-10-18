#include "win_resources.h"

// deps
#include <imgui.h>
// local
#include "app/parameters.h"
#include "assets/assets.h"
#include "events.h"
#include "imguizmo.quat/imGuIZMOquat.h"
#include "scene/scene.h"
#include "ui/base_components.h"
#include "ui/utility.h"

namespace ui {

void WinResources::ShowLoadingInfo() {
  for (size_t i = 0; i < loading_info_.models.size(); ++i) {
    ImGui::Text("Thread %llu: %s", i, loading_info_.models[i]);
  }
}

void WinResources::ShowTableCamera() {
  auto& table = table_camera_;
  auto& data = table.data;
  auto& selected = table.selected;
  static ImGuiTableFlags flags{ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable |
                               ImGuiTableFlags_ContextMenuInBody |
                               ImGuiTableFlags_Sortable};
  id::Camera current_id = scene_->camera_.GetId();
  if (ImGui::BeginTable("TableCamera", 2, flags)) {
    ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_DefaultSort);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    ui::SortTable([&data](unsigned int index, bool dir) {
      switch (index) {
        case 0:
          ui::SortNumber(data, &CameraRow::id, dir);
          break;
        case 1:
          ui::SortString(data, &CameraRow::name, dir);
          break;
        default:
          break;
      }
    });
    for (size_t row = 0; row < data.size(); row++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      // mark current Camera
      if (data[row].id == current_id) {
        ImGui::Text("-> %u", data[row].id);
      } else {
        ImGui::Text("%u", data[row].id);
      }
      ImGui::TableNextColumn();
      const bool is_selected = (selected == row);
      if (ImGui::Selectable(data[row].name, is_selected,
                            ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowDoubleClick)) {
        selected = row;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          auto id = table.GetSelected().id;
          scene_->cameras_.SetMainCamera(id);
        }
      }
    }
    ImGui::EndTable();
  }
  if (ImGui::Button("Create Camera")) {
    auto& camera = scene_->cameras_.CreateCamera(CameraConfig{});
    scene_->camera_.Use(camera);
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete Camera")) {
    if (table.data.size() > 1) {
      auto deleted_id = table.DeleteSelected().id;
      // set -> delete order (copies data back to System)
      if (current_id == deleted_id) {
        auto new_id = table.GetSelected().id;
        scene_->cameras_.SetMainCamera(new_id);
      }
      scene_->cameras_.DeleteCamera(deleted_id);
    }
  }
}

void WinResources::ShowTableModel() {
  auto& table = table_model_;
  auto& data = table.data;
  auto& selected = table.selected;
  ImGui::Text("Total Models: %llu", data.size());
  static ImGuiTableFlags flags{ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable |
                               ImGuiTableFlags_ContextMenuInBody |
                               ImGuiTableFlags_Sortable};
  if (ImGui::BeginTable("TableModel", 3, flags)) {
    ImGui::TableSetupColumn("Id");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Count");
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    ui::SortTable([&data](unsigned int index, bool dir) {
      switch (index) {
        case 0:
          ui::SortNumber(data, &ModelRow::id, dir);
          break;
        case 1:
          ui::SortString(data, &ModelRow::name, dir);
          break;
        case 2:
          ui::SortNumber(data, &ModelRow::objects_count, dir);
          break;
        default:
          break;
      }
    });
    for (size_t row = 0; row < data.size(); row++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%u", data[row].id);
      ImGui::TableNextColumn();
      const bool is_selected = (selected == row);
      if (ImGui::Selectable(data[row].name, is_selected,
                            ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowDoubleClick)) {
        selected = row;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          std::string create{table.GetSelected().name};
          scene_->CreateObjectAtCamera(create);
        }
      }
      ImGui::TableNextColumn();
      ImGui::Text("%u", data[row].objects_count);
    }
    ImGui::EndTable();
  }
}

void WinResources::ShowTableTexture() {
  auto& table = table_texture_;
  auto& data = table.data;
  auto& selected = table.selected;
  ImGui::Text("Total Textures: %llu", data.size());
  static ImGuiTableFlags flags{ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable |
                               ImGuiTableFlags_ContextMenuInBody |
                               ImGuiTableFlags_Sortable};
  if (ImGui::BeginTable("TableTexture", 5, flags)) {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("TBO");
    ImGui::TableSetupColumn("Path");
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Count");
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    ui::SortTable([&data](unsigned int index, bool dir) {
      switch (index) {
        case 0:
          ui::SortNumber(data, &TextureRow::id, dir);
          break;
        case 1:
          ui::SortNumber(data, &TextureRow::tbo, dir);
          break;
        case 2:
          ui::SortString(data, &TextureRow::path, dir);
          break;
        case 3:
          ui::SortString(data, &TextureRow::type, dir);
          break;
        case 4:
          ui::SortNumber(data, &TextureRow::count, dir);
          break;
        default:
          break;
      }
    });
    for (size_t row = 0; row < data.size(); row++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%u", data[row].id);
      ImGui::TableNextColumn();
      ImGui::Text("%u", data[row].tbo);
      ImGui::TableNextColumn();
      const bool is_selected = (selected == row);
      if (ImGui::Selectable(data[row].path, is_selected,
                            ImGuiSelectableFlags_SpanAllColumns)) {
        selected = row;
      }
      ImGui::TableNextColumn();
      ImGui::Text("%s", data[row].type);
      ImGui::TableNextColumn();
      ImGui::Text("%ld", data[row].count);
    }
    ImGui::EndTable();
  }
  if (ImGui::Button("Update Count")) {
    event::Invoke(Event::kRequestTexturesCount);
  }
}

void WinResources::ShowTableTextureEnv() {
  auto& table = table_texture_env_;
  auto& data = table.data;
  auto& selected = table.selected;
  ImGui::Text("Total Textures: %llu", data.size());
  static ImGuiTableFlags flags{ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_Resizable |
                               ImGuiTableFlags_ContextMenuInBody |
                               ImGuiTableFlags_Sortable};
  if (ImGui::BeginTable("TableTextureEnv", 2, flags)) {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Path");
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    ui::SortTable([&data](unsigned int index, bool dir) {
      switch (index) {
        case 0:
          ui::SortNumber(data, &TextureEnvRow::id, dir);
          break;
        case 1:
          ui::SortString(data, &TextureEnvRow::path, dir);
          break;
        default:
          break;
      }
    });
    for (size_t row = 0; row < data.size(); row++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%u", data[row].id);
      ImGui::TableNextColumn();
      const bool is_selected = (selected == row);
      if (ImGui::Selectable(data[row].path, is_selected,
                            ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowDoubleClick)) {
        selected = row;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          const auto& env_tex = table.GetSelected();
          scene_->env_tex_ = assets_->env_tex_.GetEnvTexture(env_tex.path);
        }
      }
    }
    ImGui::EndTable();
  }
}

void WinResources::ShowFxPresets() {
  FxPreset* fx = list_fx_.Selected().fx;
  ui::ShowComboBox(list_fx_);

  static float gizmo_size = 128.0f * app::glfw.window_scale.x;
  ImGui::gizmo3D("##emitter_gizmo", fx->emitter_dir, gizmo_size,
                 imguiGizmo::modeDirection);
  ImGui::Text("Emitter Direction [x: %.3f, y: %.3f, z: %.3f]",
              fx->emitter_dir[0], fx->emitter_dir[1], fx->emitter_dir[2]);

  ImGui::SliderFloat3("Particles Acceleration##fx", &fx->acceleration[0],
                      -10.0f, 10.0f, "%.1f");
  ImGui::SliderFloat3("Direction Constraints##fx",
                      &fx->direction_constraints[0], 0.0f, 1.0f, "%1.0f");
  ImGui::SliderFloat2("Start Position Min/Max##fx",
                      &fx->start_position_min_max[0], -5.0f, 5.0f, "%.1f");
  ImGui::SliderFloat2("Start Velocity Min/Max##fx",
                      &fx->start_velocity_min_max[0], -5.0f, 5.0f, "%.1f");

  ImGui::SliderFloat("Particle Lifetime##fx", &fx->particle_lifetime, 0.1f,
                     30.0f, "%.1f");
  ImGui::SliderFloat("Cone Angle [deg]##fx", &fx->cone_angle_deg, 0.0f, 180.0f,
                     "%.1f");
  ui::SliderUint("Number Of Particles##fx", &fx->num_of_particles, 0,
                 global::kMaxParticlesNum, "%u");

  ImGui::Text("Default values for created instances:");
  ImGui::SliderFloat2("Particle Size Min/Max##fx",
                      &fx->particle_size_min_max[0], 0.01f, 5.0f, "%.2f");
  ImGui::ColorEdit3("Color##fx", &fx->color[0]);
  ImGui::Checkbox("Fade Out##fx", &fx->should_fade_out);

  if (ImGui::Button("Update##fx")) {
    event::Invoke(Event::kUpdateFx);
    event::Invoke(Event::kUpdateParticlesInstances);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset##fx")) {
    event::Invoke(Event::kResetFx);
    event::Invoke(Event::kUpdateParticlesInstances);
  }
}

void WinResources::MenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load model from file [TODO]")) {
        //
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void WinResources::RequestTextureCount() {
  auto id_to_count = assets_->textures_.CalcTextureUseCount();
  for (auto& row : table_texture_.data) {
    long count = id_to_count.at(row.id);
    row.count = count;
  }
}

void WinResources::RequestObjectsCount() {
  auto counter = scene_->objects_.CalcObjectsPerModel();
  for (auto& row : table_model_.data) {
    auto it = counter.find(row.id);
    if (it != counter.end()) {
      row.objects_count = it->second;
    } else {
      row.objects_count = 0;
    }
  }
}

void WinResources::TabBar() {
  if (ImGui::BeginTabBar("TabBarResources")) {
    if (ImGui::BeginTabItem("Cameras")) {
      ShowTableCamera();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Models")) {
      ShowTableModel();
      ImGui::EndTabItem();
    }
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      RequestObjectsCount();
    }
    if (ImGui::BeginTabItem("Textures")) {
      ShowTableTexture();
      ImGui::EndTabItem();
    }
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      RequestTextureCount();
    }
    if (ImGui::BeginTabItem("Textures Env")) {
      ShowTableTextureEnv();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("FX Presets (Particles)")) {
      ShowFxPresets();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

}  // namespace ui