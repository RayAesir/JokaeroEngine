#pragma once

// deps
#include <imgui.h>

namespace ui {

void ShowLoadingIndicatorCircle(const char* label, const float radius,
                                const ImVec4& main_color,
                                const ImVec4& backdrop_color,
                                const int circle_count, const float speed);

}  // namespace ui