#pragma once

// deps
#include <imgui.h>
// global
#include <algorithm>
#include <functional>

namespace ui {

class Delay {
 public:
  Delay(float ms);
  void Wait();
  void Done();
  operator bool() const;

 private:
  float wait_{0.0f};
  float counter_{0.0f};
};

struct WinCorner {
  enum Enum : int { kTopLeft, kTopRight, kBottomLeft, kBottomRight };
};

struct WinCenter {
  enum Enum : int { kCenter, kBottom, kTop };
};

void SetCornerWindowPos(int corner, int im_gui_cond, ImVec2 pad);
void SetCenterWindowPos(int center, int im_gui_cond, float pad_y);

// sort a table by a single column
void SortTable(const std::function<void(unsigned int, bool)>& fun);

template <typename T, typename C, typename R>
void SortNumber(T& container, R(C::*field), bool dir) {
  std::sort(container.begin(), container.end(),
            [&dir, &field](const C& l, const C& r) {
              return dir ? (l.*field < r.*field) : (l.*field > r.*field);
            });
}

template <typename T, typename C, typename R>
void SortString(T& container, R(C::*field), bool dir) {
  std::sort(container.begin(), container.end(),
            [&dir, &field](const C& l, const C& r) {
              int result = std::strcmp(l.*field, r.*field);
              // reverse a result for the descending order
              if (dir == false) result *= -1;
              // negative precedes, zero equal, positive succeeds
              bool precedes = (result < 0) ? true : false;
              return precedes;
            });
}

}  // namespace ui