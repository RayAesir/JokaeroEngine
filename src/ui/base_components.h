#pragma once

// deps
#include <imgui.h>
// local
#include "ui/base_types.h"

namespace ui {

template <typename T>
bool ShowComboBox(Selectable<T>& data) {
  bool selected = false;
  const char* preview_mode = data.list[data.current].str.c_str();
  if (ImGui::BeginCombo(data.label.c_str(), preview_mode)) {
    for (int i = 0; i < static_cast<int>(data.list.size()); ++i) {
      const bool is_selected = (data.current == i);
      if (ImGui::Selectable(data.list[i].str.c_str(), is_selected)) {
        data.current = i;
        selected = true;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  return selected;
}

template <typename T>
bool ShowListBox(Selectable<T>& data) {
  bool selected = false;
  if (ImGui::BeginListBox(data.label.c_str())) {
    for (int i = 0; i < static_cast<int>(data.list.size()); ++i) {
      const bool is_selected = (data.current == i);
      if (ImGui::Selectable(data.list[i].str.c_str(), is_selected)) {
        data.current = i;
        selected = true;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndListBox();
  }
  return selected;
}

inline bool SliderUint(const char* label, ImU32* value, ImU32 min = 0,
                       ImU32 max = 0, const char* format = "%u") {
  return ImGui::SliderScalar(label, ImGuiDataType_U32, value, &min, &max,
                             format);
}

}  // namespace ui