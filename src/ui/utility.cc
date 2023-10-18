#include "utility.h"

namespace ui {

Delay::Delay(float ms) : wait_(ms / 1000.0f) {}

void Delay::Wait() { counter_ += ImGui::GetIO().DeltaTime; }

void Delay::Done() { counter_ = 0.0f; }

Delay::operator bool() const { return (counter_ >= wait_); }

void SortTable(const std::function<void(unsigned int, bool)>& fun) {
  if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
    if (sort_specs->SpecsDirty) {
      for (int n = 0; n < sort_specs->SpecsCount; ++n) {
        const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[n];
        bool dir = (sort_spec->SortDirection == ImGuiSortDirection_Ascending)
                       ? true
                       : false;
        // call sorting callback
        fun(sort_spec->ColumnIndex, dir);
      }
      sort_specs->SpecsDirty = false;
    }
  }
}

void SetCornerWindowPos(int corner, int im_gui_cond, ImVec2 pad) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  // work area without MainMenuBar
  const ImVec2& work_pos = viewport->WorkPos;
  const ImVec2& work_size = viewport->WorkSize;
  ImVec2 window_pos;
  ImVec2 window_pos_pivot;
  // right = true, left = false
  window_pos.x =
      (corner & 1) ? (work_pos.x + work_size.x - pad.x) : (work_pos.x + pad.x);
  // bottom = true, top = false
  window_pos.y =
      (corner & 2) ? (work_pos.y + work_size.y - pad.y) : (work_pos.y + pad.y);
  window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
  window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
  ImGui::SetNextWindowPos(window_pos, im_gui_cond, window_pos_pivot);
}

void SetCenterWindowPos(int center, int im_gui_cond, float pad_y) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2& work_center = viewport->GetWorkCenter();
  // work area without MainMenuBar
  const ImVec2& work_pos = viewport->WorkPos;
  const ImVec2& work_size = viewport->WorkSize;
  // center by default
  ImVec2 window_pos = work_center;
  ImVec2 window_pos_pivot{0.5f, 0.5f};
  if (center == WinCenter::kBottom) {
    window_pos.y = work_pos.y + work_size.y - pad_y;
    window_pos_pivot.y = 1.0f;
  }
  if (center == WinCenter::kTop) {
    window_pos.y = work_pos.y + pad_y;
    window_pos_pivot.y = 0.0f;
  }

  ImGui::SetNextWindowPos(window_pos, im_gui_cond, window_pos_pivot);
}

}  // namespace ui