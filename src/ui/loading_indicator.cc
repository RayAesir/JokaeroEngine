#include "loading_indicator.h"

// deps
#include <imgui_internal.h>
// global
#include <cmath>

namespace ui {

// by alexsr
void ShowLoadingIndicatorCircle(const char* label, const float indicator_radius,
                                const ImVec4& main_color,
                                const ImVec4& backdrop_color,
                                const int circle_count, const float speed) {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return;
  }

  ImGuiContext& g = *GImGui;
  const ImGuiID id = window->GetID(label);

  const ImVec2 pos = window->DC.CursorPos;
  const float circle_radius = indicator_radius / 10.0f;
  const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f,
                              pos.y + indicator_radius * 2.0f));
  ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);
  if (!ImGui::ItemAdd(bb, id)) {
    return;
  }

  const float t = static_cast<float>(g.Time);
  const float degree_offset = 2.0f * IM_PI / circle_count;
  for (int i = 0; i < circle_count; ++i) {
    const float x = indicator_radius * std::sin(degree_offset * i);
    const float y = indicator_radius * std::cos(degree_offset * i);
    const float growth =
        std::max(0.0f, std::sin(t * speed - i * degree_offset));
    ImVec4 color;
    color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
    color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
    color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
    color.w = main_color.w * growth + backdrop_color.w * (1.0f - growth);
    // color.w = 1.0f;
    window->DrawList->AddCircleFilled(
        ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
        circle_radius + growth * circle_radius, ImGui::GetColorU32(color));
  }
}

}  // namespace ui
